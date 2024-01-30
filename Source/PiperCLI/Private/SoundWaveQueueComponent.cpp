#include "SoundWaveQueueComponent.h"
#include "TimerManager.h"

USoundWaveQueueComponent::USoundWaveQueueComponent(const FObjectInitializer& init) : UActorComponent(init)
{
}

USoundWaveQueueComponent::~USoundWaveQueueComponent()
{
}

void USoundWaveQueueComponent::QueueSound(USoundWave* Sound)
{
}

void USoundWaveQueueComponent::Stop()
{
}

void USoundWaveQueueComponent::ClearQueue()
{
}

void USoundWaveQueueComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void USoundWaveQueueComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void USoundWaveQueueComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USoundWaveQueueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}
