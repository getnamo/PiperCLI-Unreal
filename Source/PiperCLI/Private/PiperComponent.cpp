#include "PiperComponent.h"

void UPiperComponent::InitializeComponent()
{
	Super::InitializeComponent();
	ProcessHandler = MakeShareable(new FSubProcessHandler);
}

void UPiperComponent::UninitializeComponent()
{
	ProcessHandler = nullptr;
	Super::UninitializeComponent();
}

void UPiperComponent::BeginPlay()
{
	Super::BeginPlay();
}

UPiperComponent::UPiperComponent(const FObjectInitializer& init) : UActorComponent(init)
{
}

UPiperComponent::~UPiperComponent()
{

}
