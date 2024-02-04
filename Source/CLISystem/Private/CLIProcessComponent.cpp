#include "CLIProcessComponent.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"


UCLIProcessComponent::UCLIProcessComponent(const FObjectInitializer& init) : UActorComponent(init)
{
	bWantsInitializeComponent = true;
	bAutoActivate = true;

	//Main options
	CLIParams.OptionalWorkingDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/PiperCLI-Unreal/ThirdParty/Example"));
	CLIParams.Url = CLIParams.OptionalWorkingDirectory  + TEXT("process.exe");
	CLIParams.bProcessInBytes = false;
	
	//By default wrap process into our unreal process
	CLIParams.bLaunchHidden = true;
	CLIParams.bLaunchReallyHidden = true;
}

UCLIProcessComponent::~UCLIProcessComponent()
{

}

void UCLIProcessComponent::SendInput(const FString& Text)
{
	if (bLazyAutoStartProcess && !bProcessIsRunning)
	{
		StartProcess();
	}
	ProcessHandler->SendInput(Text);
}

void UCLIProcessComponent::StartProcess()
{
	if (ProcessHandler)
	{
		ProcessHandler->StartProcess(CLIParams);
	}
}

void UCLIProcessComponent::StopProcess()
{
	if (ProcessHandler)
	{
		ProcessHandler->StopProcess();
	}
}

void UCLIProcessComponent::InitializeComponent()
{
	Super::InitializeComponent();
	ProcessHandler = MakeShareable(new FSubProcessHandler());

	ProcessHandler->OnProcessBegin = [this](const int32 ProcessId, bool bStartSucceded)
	{
		bProcessIsRunning = true;
		OnBeginProcessing.Broadcast(FString::Printf(TEXT("Startup Success: %d"), bStartSucceded));
	};

	ProcessHandler->OnProcessOutput = [this](const int32 ProcessId, const FString & OutputString)
	{
		OnOutputText.Broadcast(OutputString);
	};

	ProcessHandler->OnProcessOutputBytes = [this](const int32 ProcessId, const TArray<uint8>& OutputBytes)
	{
		OnOutputBytes.Broadcast(OutputBytes);
	};

	ProcessHandler->OnProcessEnd = [this](const int32 ProcessId, int32 ReturnCode)
	{
		bProcessIsRunning = false;
		OnEndProcessing.Broadcast(FString::Printf(TEXT("ReturnCode: %d"), ReturnCode));
	};
}

void UCLIProcessComponent::UninitializeComponent()
{
	ProcessHandler = nullptr;
	bProcessIsRunning = false;
	Super::UninitializeComponent();
}

void UCLIProcessComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bStartProcessOnBeginPlay)
	{
		StartProcess();
	}
}

void UCLIProcessComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ProcessHandler)
	{
		ProcessHandler->StopProcess();
	}
	Super::EndPlay(EndPlayReason);
}