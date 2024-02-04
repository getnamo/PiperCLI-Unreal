#include "PiperComponent.h"
#include "Audio.h"
#include "Async/Async.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"


UPiperComponent::UPiperComponent(const FObjectInitializer& init) : UCLIProcessComponent(init)
{
	bWantsInitializeComponent = true;
	bAutoActivate = true;

	//Main options
	CLIParams.OptionalWorkingDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/PiperCLI-Unreal/ThirdParty/Piper/Win64/"));
	CLIParams.Url = CLIParams.OptionalWorkingDirectory  + TEXT("piper.exe");
	CLIParams.bProcessInBytes = true;	//We process buffers directly for efficiency
	
	CLIParams.bLaunchHidden = true;
	CLIParams.bLaunchReallyHidden = true;
	CLIParams.bOutputToGameThread = false;

	SyncCLIParams();
}

UPiperComponent::~UPiperComponent()
{

}



void UPiperComponent::SyncCLIParams()
{
	if (!PiperParams.bSyncCLIParams) 
	{
		return;
	}

	//Sync Basic model
	CLIParams.Params = FString::Printf(TEXT("--model ./model/%s"), *PiperParams.VoiceModelName);

	//append options
	if (PiperParams.bOutputToUnreal)
	{
		CLIParams.Params += TEXT(" --output-raw");
	}
	if (PiperParams.bUseJsonFormatInput)
	{
		CLIParams.Params += TEXT(" --json-input");
	}
}

TArray<uint8> UPiperComponent::PCMToWav(const TArray<uint8>& InPCMBytes, int32 SampleRate, int32 Channels)
{
	TArray<uint8> WavBytes;
	SerializeWaveFile(WavBytes, InPCMBytes.GetData(), InPCMBytes.Num(), Channels, SampleRate);
	return WavBytes;
}

void UPiperComponent::SetSoundWaveFromWavBytes(USoundWaveProcedural* InSoundWave, const TArray<uint8>& InBytes)
{
	FWaveModInfo WaveInfo;

	FString ErrorReason;
	if (WaveInfo.ReadWaveInfo(InBytes.GetData(), InBytes.Num(), &ErrorReason))
	{
		//copy header info
		int32 DurationDiv = *WaveInfo.pChannels * *WaveInfo.pBitsPerSample * *WaveInfo.pSamplesPerSec;
		if (DurationDiv)
		{
			InSoundWave->Duration = *WaveInfo.pWaveDataSize * 8.0f / DurationDiv;
		}
		else
		{
			InSoundWave->Duration = 0.0f;
		}

		InSoundWave->SetSampleRate(*WaveInfo.pSamplesPerSec);
		InSoundWave->NumChannels = *WaveInfo.pChannels;
		//SoundWaveProc->RawPCMDataSize = WaveInfo.SampleDataSize;
		InSoundWave->bLooping = false;
		InSoundWave->SoundGroup = ESoundGroup::SOUNDGROUP_Default;

		//Queue actual audio data
		InSoundWave->QueueAudio(WaveInfo.SampleDataStart, WaveInfo.SampleDataSize);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("SetSoundWaveFromWavBytes::WaveRead error: %s"), *ErrorReason);
	}
}

USoundWave* UPiperComponent::WavToSoundWave(const TArray<uint8>& InWavBytes)
{
	USoundWave* SoundWave;

	//Allocate based on thread
	if (IsInGameThread())
	{
		SoundWave = NewObject<USoundWaveProcedural>(USoundWaveProcedural::StaticClass());
		SetSoundWaveFromWavBytes((USoundWaveProcedural*)SoundWave, InWavBytes);
	}
	else
	{
		//We will go to another thread, copy our bytes
		TArray<uint8> CopiedBytes = InWavBytes;
		FThreadSafeBool bAllocationComplete = false;
		AsyncTask(ENamedThreads::GameThread, [&bAllocationComplete, &SoundWave]
		{
			SoundWave = NewObject<USoundWaveProcedural>(USoundWaveProcedural::StaticClass());
			bAllocationComplete = true;
		});

		//block while not complete
		while (!bAllocationComplete)
		{
			//100micros sleep, this should be very quick
			FPlatformProcess::Sleep(0.0001f);
		};

		SetSoundWaveFromWavBytes((USoundWaveProcedural*)SoundWave, CopiedBytes);
	}

	return SoundWave;
}

void UPiperComponent::StartProcess()
{
	//Ensure these are synced before we start
	SyncCLIParams();

	Super::StartProcess();
}

void UPiperComponent::InitializeComponent()
{
	Super::InitializeComponent();

	ProcessHandler->OnProcessOutput = [this](const int32 ProcessId, const FString& OutputString)
	{
		//Because we do not process bytes on game thread we need this wrapper here
		AsyncTask(ENamedThreads::GameThread, [&]
		{
			OnOutputText.Broadcast(OutputString);
		});
	};

	//We handle byte output processing differently for piper, auto-categorize output as text or string based on string length
	ProcessHandler->OnProcessOutputBytes = [this](const int32 ProcessId, const TArray<uint8>& OutputBytes)
	{
		FString ResultString;
		FFileHelper::BufferToString(ResultString, OutputBytes.GetData(), OutputBytes.Num());

		//This was a text message, not a piper pcm buffer
		if (ResultString.Len() == OutputBytes.Num())
		{
			AsyncTask(ENamedThreads::GameThread, [&, ResultString]
			{
				OnOutputText.Broadcast(ResultString);
			});
		}
		else if (PiperParams.bOutputSoundWaves)
		{
			//Convert to Wavbytes on bg thread
			TArray<uint8> WavBytes = PCMToWav(OutputBytes, PiperParams.SampleRate, PiperParams.Channels);

			//Convert and emit on game thread - todo: optimization of pre-gen soundwave on game thread, then just async convert
			AsyncTask(ENamedThreads::GameThread, [&, WavBytes]
			{
				//Convert to usoundwave
				USoundWave* Sound = WavToSoundWave(WavBytes);

				OnAudioGenerated.Broadcast(Sound);
			});
		}
		else
		{
			//Copy bytes
			TArray<uint8> SafeBytes = OutputBytes;
			AsyncTask(ENamedThreads::GameThread, [&, SafeBytes]
			{
				OnOutputBytes.Broadcast(OutputBytes);
			});
		}
	};
}

void UPiperComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void UPiperComponent::BeginPlay()
{
	Super::BeginPlay();
}


void UPiperComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}