#include "PiperComponent.h"
#include "Misc/Paths.h"


UPiperComponent::UPiperComponent(const FObjectInitializer& init) : UActorComponent(init)
{
	PiperCLIParams.Url = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("ThirdParty/Piper/Win64/piper.exe"));
}

UPiperComponent::~UPiperComponent()
{

}


void UPiperComponent::InitializeComponent()
{
	Super::InitializeComponent();
	ProcessHandler = MakeShareable(new FSubProcessHandler);

	ProcessHandler->OnProcessBegin = [this](const int32 ProcessId, bool bStartSucceded)
	{
		OnBeginProcessing.Broadcast(FString::Printf(TEXT("Sucess: %d"), bStartSucceded));
	};

	ProcessHandler->OnProcessOutput = [this](const int32 ProcessId, const FString & OutputString)
	{
		OnOutput.Broadcast(OutputString);
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
		ProcessHandler->StartProcess(PiperCLIParams);
	}
}


void UPiperComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ProcessHandler->StopProcess();
	Super::EndPlay(EndPlayReason);
}
