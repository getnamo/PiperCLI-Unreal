#include "PiperComponent.h"
#include "Misc/Paths.h"


UPiperComponent::UPiperComponent(const FObjectInitializer& init) : UActorComponent(init)
{
	bWantsInitializeComponent = true;
	bAutoActivate = true;

	PiperCLIParams.OptionalWorkingDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/PiperCLI-Unreal/ThirdParty/Piper/Win64/"));
	PiperCLIParams.Url = PiperCLIParams.OptionalWorkingDirectory  + TEXT("piper.exe");
	PiperCLIParams.bLaunchHidden = true;
	PiperCLIParams.bLaunchReallyHidden = true;
}

UPiperComponent::~UPiperComponent()
{

}


void UPiperComponent::SendInput(const FString& Text)
{
	ProcessHandler->SendInput(Text);
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
		//Lame separator for now
		//if (OutputBytes.Num() > 512) 
		//{
			OnOutputBytes.Broadcast(OutputBytes);
		//}
		//else
		//{
		//	OnOutput.Broadcast(FString(OutputBytes.Num(), (UTF8CHAR*)OutputBytes.GetData()));
		//}
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
		if (ProcessHandler) 
		{
			ProcessHandler->StartProcess(PiperCLIParams);
		}
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
