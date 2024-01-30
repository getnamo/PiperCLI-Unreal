#include "PiperComponent.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"


UPiperComponent::UPiperComponent(const FObjectInitializer& init) : UActorComponent(init)
{
	bWantsInitializeComponent = true;
	bAutoActivate = true;

	//Main options
	PiperCLIParams.OptionalWorkingDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/PiperCLI-Unreal/ThirdParty/Piper/Win64/"));
	PiperCLIParams.Url = PiperCLIParams.OptionalWorkingDirectory  + TEXT("piper.exe");
	
	PiperCLIParams.bLaunchHidden = true;
	PiperCLIParams.bLaunchReallyHidden = true;

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
	PiperCLIParams.Params = FString::Printf(TEXT("--model ./model/%s"), *PiperParams.VoiceModelName);

	//append options
	if (PiperParams.bOutputToUnreal)
	{
		PiperCLIParams.Params += TEXT(" --output-raw");
	}
	if (PiperParams.bUseJsonFormatInput)
	{
		PiperCLIParams.Params += TEXT(" --json-input");
	}
}



void UPiperComponent::SendInput(const FString& Text)
{
	ProcessHandler->SendInput(Text);
}

void UPiperComponent::StartPiperProcess()
{
	//Ensure these are synced before we start
	SyncCLIParams();

	if (ProcessHandler)
	{
		ProcessHandler->StartProcess(PiperCLIParams);
	}
}

void UPiperComponent::StopPiperProcess()
{
	if (ProcessHandler)
	{
		ProcessHandler->StopProcess();
	}
}

void UPiperComponent::InitializeComponent()
{
	Super::InitializeComponent();
	ProcessHandler = MakeShareable(new FSubProcessHandler());

	ProcessHandler->OnProcessBegin = [this](const int32 ProcessId, bool bStartSucceded)
	{
		OnBeginProcessing.Broadcast(FString::Printf(TEXT("Startup Success: %d"), bStartSucceded));
	};

	ProcessHandler->OnProcessOutput = [this](const int32 ProcessId, const FString & OutputString)
	{
		OnOutput.Broadcast(OutputString);
	};

	ProcessHandler->OnProcessOutputBytes = [this](const int32 ProcessId, const TArray<uint8>& OutputBytes)
	{
		FString ResultString;
		FFileHelper::BufferToString(ResultString, OutputBytes.GetData(), OutputBytes.Num());

		if (ResultString.Len() == OutputBytes.Num())
		{
			OnOutput.Broadcast(ResultString);
		}
		else
		{
			OnOutputBytes.Broadcast(OutputBytes);
		}
	};

	ProcessHandler->OnProcessEnd = [this](const int32 ProcessId, int32 ReturnCode)
	{
		OnEndProcessing.Broadcast(FString::Printf(TEXT("ReturnCode: %d"), ReturnCode));
	};
}

void UPiperComponent::UninitializeComponent()
{
	ProcessHandler = nullptr;
	Super::UninitializeComponent();
}

void UPiperComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bStartPiperOnBeginPlay)
	{
		StartPiperProcess();
	}
}


void UPiperComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ProcessHandler)
	{
		ProcessHandler->StopProcess();
	}
	Super::EndPlay(EndPlayReason);
}

