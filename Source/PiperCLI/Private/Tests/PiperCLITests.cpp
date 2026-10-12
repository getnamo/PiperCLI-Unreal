// Automation tests for UPiperComponent against the real piper.exe. Run headless:
//   UnrealEditor-Cmd.exe <Project>.uproject -ExecCmds="Automation RunTests PiperCLI; Quit" -unattended -nullrhi -nopause
// They need piper and at least one voice model in Plugins/PiperCLI-Unreal/ThirdParty/Piper/Win64/, and
// are skipped with a warning otherwise.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PiperComponent.h"
#include "PiperCLITestReceiver.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace PiperCLITests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
	constexpr double Timeout = 30.0;

	struct FVoice
	{
		FString Model;
		int32 SampleRate = 22050;
	};

	/** Prefers en_US-kristin-medium, else the first model; reads its sample rate from the .onnx.json. */
	static bool FindVoice(FVoice& Out)
	{
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/PiperCLI-Unreal/ThirdParty/Piper/Win64/"));
		if (!FPaths::FileExists(Dir + TEXT("piper.exe")))
		{
			return false;
		}
		TArray<FString> Models;
		IFileManager::Get().FindFiles(Models, *(Dir + TEXT("model/*.onnx")), true, false);
		if (Models.Num() == 0)
		{
			return false;
		}
		Models.Sort();
		Out.Model = Models.Contains(TEXT("en_US-kristin-medium.onnx")) ? FString(TEXT("en_US-kristin-medium.onnx")) : Models[0];

		FString Json;
		TSharedPtr<FJsonObject> Config;
		if (FFileHelper::LoadFileToString(Json, *(Dir + TEXT("model/") + Out.Model + TEXT(".json"))) &&
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Config) && Config.IsValid())
		{
			const TSharedPtr<FJsonObject>* Audio = nullptr;
			if (Config->TryGetObjectField(TEXT("audio"), Audio))
			{
				(*Audio)->TryGetNumberField(TEXT("sample_rate"), Out.SampleRate);
			}
		}
		return true;
	}

	struct FEnv
	{
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		UPiperCLITestReceiver* Receiver = nullptr;
		TWeakObjectPtr<UPiperComponent> Piper;
		double SentTime = 0.0;

		void Create()
		{
			GameInstance = NewObject<UGameInstance>(GEngine);
			GameInstance->AddToRoot();
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
			World->InitializeActorsForPlay(FURL());
			Receiver = NewObject<UPiperCLITestReceiver>();
			Receiver->AddToRoot();
		}

		/** Configure runs before registration/BeginPlay, like editing the component's details. */
		UPiperComponent* AddPiper(const FVoice& Voice, TFunctionRef<void(UPiperComponent*)> Configure)
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UPiperComponent* C = NewObject<UPiperComponent>(Actor);
			C->PiperParams.VoiceModelName = Voice.Model;
			C->PiperParams.SampleRate = Voice.SampleRate;
			C->PiperParams.ByteOutputSampleRate = Voice.SampleRate;
			Configure(C);
			C->OnAudioGenerated.AddDynamic(Receiver, &UPiperCLITestReceiver::OnAudio);
			C->OnOutputBytes.AddDynamic(Receiver, &UPiperCLITestReceiver::OnBytes);
			C->OnOutputText.AddDynamic(Receiver, &UPiperCLITestReceiver::OnText);
			C->OnBeginProcessing.AddDynamic(Receiver, &UPiperCLITestReceiver::OnBegin);
			C->OnEndProcessing.AddDynamic(Receiver, &UPiperCLITestReceiver::OnEnd);
			C->RegisterComponent();
			Actor->DispatchBeginPlay(); //bStartProcessOnBeginPlay
			Piper = C;
			return C;
		}

		void Destroy()
		{
			if (UPiperComponent* C = Piper.Get())
			{
				C->StopProcess();
				C->GetOwner()->Destroy();
			}
			GameInstance->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			GameInstance->RemoveFromRoot();
			Receiver->RemoveFromRoot();
		}
	};

	static void Step(TFunction<void()> Fn)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Fn]() { Fn(); return true; }));
	}

	static void WaitUntil(FAutomationTestBase* Test, const FString& Label, TFunction<bool()> Condition, double InTimeout = Timeout)
	{
		const double Deadline = FPlatformTime::Seconds() + InTimeout;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Test, Label, Condition, Deadline]() -> bool
		{
			if (Condition())
			{
				return true;
			}
			if (FPlatformTime::Seconds() > Deadline)
			{
				Test->AddError(TEXT("Timed out waiting for ") + Label);
				return true;
			}
			return false;
		}));
	}

	/** Audio has arrived (at least Count clips) and nothing new for QuietSeconds: piper finished the input. */
	static void WaitForAudio(FAutomationTestBase* Test, TSharedRef<FEnv> E, int32 Count, double QuietSeconds = 1.5)
	{
		WaitUntil(Test, FString::Printf(TEXT("%d audio clip(s)"), Count), [E, Count, QuietSeconds]
		{
			return E->Receiver->Sounds.Num() >= Count && FPlatformTime::Seconds() - E->Receiver->LastAudioTime > QuietSeconds;
		});
	}

	static void Delay(float Seconds)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([] {}, Seconds));
	}
}

using namespace PiperCLITests;

// Input sent in the same frame as BeginPlay (before the process is up) is spoken, with one process and the right transcript.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPiperSendBeforeStartTest, "PiperCLI.Component.SendBeforeStart", PiperCLITests::Flags)
bool FPiperSendBeforeStartTest::RunTest(const FString&)
{
	FVoice Voice;
	if (!FindVoice(Voice)) { AddWarning(TEXT("piper.exe or a voice model is missing; skipped")); return true; }

	TSharedRef<FEnv> E = MakeShared<FEnv>();
	E->Create();
	UPiperComponent* Piper = E->AddPiper(Voice, [](UPiperComponent*) {});
	const FString First = TEXT("This line was sent before piper had started.");
	Piper->SendInput(First);

	WaitForAudio(this, E, 1);
	Step([this, E, First]
	{
		TestEqual(TEXT("one piper process started"), E->Receiver->Begins, 1);
		if (E->Receiver->Sounds.Num() > 0)
		{
			TestEqual(TEXT("first clip carries its transcript"), E->Receiver->Sounds[0].Transcript, First);
		}
		E->Receiver->Sounds.Reset();
		E->Piper->SendInput(TEXT("A second line."));
	});
	WaitForAudio(this, E, 1);
	Step([this, E]
	{
		if (E->Receiver->Sounds.Num() > 0)
		{
			TestEqual(TEXT("second clip carries its own transcript"), E->Receiver->Sounds[0].Transcript, FString(TEXT("A second line.")));
		}
		TestEqual(TEXT("still one piper process"), E->Receiver->Begins, 1);
		E->Destroy();
	});
	return true;
}

// Byte output is the resampled PCM, and the SoundWave is built from the original PCM at the voice's rate.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPiperByteOutputTest, "PiperCLI.Component.ByteOutputResampled", PiperCLITests::Flags)
bool FPiperByteOutputTest::RunTest(const FString&)
{
	FVoice Voice;
	if (!FindVoice(Voice)) { AddWarning(TEXT("piper.exe or a voice model is missing; skipped")); return true; }

	constexpr int32 TargetRate = 16000;
	TSharedRef<FEnv> E = MakeShared<FEnv>();
	E->Create();
	E->AddPiper(Voice, [](UPiperComponent* C)
	{
		C->PiperParams.bOutputBytes = true;
		C->PiperParams.ByteOutputSampleRate = TargetRate;
	});
	WaitUntil(this, TEXT("piper running"), [E] { return E->Piper.IsValid() && E->Piper->bProcessIsRunning; });
	Step([E] { E->Piper->SendInput(TEXT("Checking the resampled byte output against the sound wave.")); });
	WaitForAudio(this, E, 1);
	Step([this, E, Voice]
	{
		UPiperCLITestReceiver* R = E->Receiver;
		if (!TestTrue(TEXT("got a clip and byte output"), R->Sounds.Num() > 0 && R->ByteOutputs.Num() > 0)) { E->Destroy(); return; }

		int64 ByteTotal = 0;
		for (const TArray<uint8>& B : R->ByteOutputs) { ByteTotal += B.Num(); }
		int64 PCMTotal = 0;
		double Seconds = 0.0;
		for (const FSWSoundData& S : R->Sounds) { PCMTotal += S.PCMBytes.Num(); Seconds += S.Duration; }

		TestEqual(TEXT("OnOutputBytes delivered the same PCM as SoundData.PCMBytes"), ByteTotal, PCMTotal);
		TestTrue(TEXT("byte output matches between events"), R->ByteOutputs.Num() == R->Sounds.Num() && R->ByteOutputs[0] == R->Sounds[0].PCMBytes);
		const double ExpectedBytes = Seconds * TargetRate * 2.0;
		AddInfo(FString::Printf(TEXT("%.2f s of audio at %d Hz, %lld PCM bytes at %d Hz (expected ~%.0f)"), Seconds, Voice.SampleRate, PCMTotal, TargetRate, ExpectedBytes));
		TestTrue(TEXT("SoundWave duration and resampled PCM length agree"), FMath::Abs(PCMTotal - ExpectedBytes) <= ExpectedBytes * 0.01 + 64);
		TestEqual(TEXT("SoundData reports the byte output rate"), R->Sounds[0].ByteOutputSampleRate, TargetRate);
		E->Destroy();
	});
	return true;
}

// A sentence interrupted by StopProcess must not shift the transcripts of later clips.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPiperStopMidSentenceTest, "PiperCLI.Component.StopMidSentence", PiperCLITests::Flags)
bool FPiperStopMidSentenceTest::RunTest(const FString&)
{
	FVoice Voice;
	if (!FindVoice(Voice)) { AddWarning(TEXT("piper.exe or a voice model is missing; skipped")); return true; }

	TSharedRef<FEnv> E = MakeShared<FEnv>();
	E->Create();
	E->AddPiper(Voice, [](UPiperComponent*) {});
	WaitUntil(this, TEXT("piper running"), [E] { return E->Piper.IsValid() && E->Piper->bProcessIsRunning; });
	Step([E] { E->Piper->SendInput(TEXT("This long sentence is interrupted part of the way through, because the process gets stopped while it is still being synthesized.")); });
	Delay(0.2f);
	Step([E] { E->Piper->StopProcess(); });
	WaitUntil(this, TEXT("piper stopped"), [E] { return !E->Piper->bProcessIsRunning; });
	Delay(0.5f);
	Step([E]
	{
		E->Receiver->Sounds.Reset();
		E->Piper->StartProcess();
		E->Piper->SendInput(TEXT("After the restart."));
	});
	WaitForAudio(this, E, 1);
	Step([this, E]
	{
		if (E->Receiver->Sounds.Num() > 0)
		{
			TestEqual(TEXT("clip after the restart carries its own transcript"), E->Receiver->Sounds.Last().Transcript, FString(TEXT("After the restart.")));
		}
		E->Destroy();
	});
	return true;
}

// Destroying the component while piper is synthesizing must not crash, and nothing may be broadcast afterwards.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPiperDestroyWhileSynthesizingTest, "PiperCLI.Component.DestroyWhileSynthesizing", PiperCLITests::Flags)
bool FPiperDestroyWhileSynthesizingTest::RunTest(const FString&)
{
	FVoice Voice;
	if (!FindVoice(Voice)) { AddWarning(TEXT("piper.exe or a voice model is missing; skipped")); return true; }

	TSharedRef<FEnv> E = MakeShared<FEnv>();
	E->Create();
	E->AddPiper(Voice, [](UPiperComponent* C) { C->PiperParams.bOutputBytes = true; });
	WaitUntil(this, TEXT("piper running"), [E] { return E->Piper.IsValid() && E->Piper->bProcessIsRunning; });
	Step([E]
	{
		E->Piper->SendInput(TEXT("First sentence of several. Second sentence of several. Third sentence of several. Fourth sentence of several."));
	});
	WaitUntil(this, TEXT("first clip"), [E] { return E->Receiver->Sounds.Num() > 0; });
	TSharedRef<int32> EventsAtDestroy = MakeShared<int32>(0);
	Step([E, EventsAtDestroy]
	{
		UPiperComponent* C = E->Piper.Get();
		AActor* Owner = C->GetOwner();
		C->DestroyComponent(); //UninitializeComponent releases the process handler mid-synthesis
		Owner->Destroy();
		*EventsAtDestroy = E->Receiver->NumEvents();
	});
	Delay(2.0f);
	Step([this, E, EventsAtDestroy]
	{
		TestEqual(TEXT("no events after the component was destroyed"), E->Receiver->NumEvents(), *EventsAtDestroy);
		E->Destroy();
	});
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
