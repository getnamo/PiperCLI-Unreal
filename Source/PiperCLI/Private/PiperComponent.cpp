#include "PiperComponent.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"


UPiperComponent::UPiperComponent(const FObjectInitializer& init) : UActorComponent(init)
{
	bWantsInitializeComponent = true;
	bAutoActivate = true;

	//Main options
	CLIParams.OptionalWorkingDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/PiperCLI-Unreal/ThirdParty/Piper/Win64/"));
	CLIParams.Url = CLIParams.OptionalWorkingDirectory  + TEXT("piper.exe");
	CLIParams.bProcessInBytes = true;	//We process buffers directly for efficiency
	
	CLIParams.bLaunchHidden = true;
	CLIParams.bLaunchReallyHidden = true;

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



void UPiperComponent::SendInput(const FString& Text)
{
	if (bLazyAutoStartProcess && !bPiperProcessRunning)
	{
		StartPiperProcess();
	}
	ProcessHandler->SendInput(Text);
}

void UPiperComponent::StartPiperProcess()
{
	//Ensure these are synced before we start
	SyncCLIParams();

	if (ProcessHandler)
	{
		ProcessHandler->StartProcess(CLIParams);
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
		bPiperProcessRunning = true;
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
		bPiperProcessRunning = false;
		OnEndProcessing.Broadcast(FString::Printf(TEXT("ReturnCode: %d"), ReturnCode));
	};
}

void UPiperComponent::UninitializeComponent()
{
	ProcessHandler = nullptr;
	bPiperProcessRunning = false;
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

