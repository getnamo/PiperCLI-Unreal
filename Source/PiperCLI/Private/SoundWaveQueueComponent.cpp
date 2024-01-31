#include "SoundWaveQueueComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

USoundWaveQueueComponent::USoundWaveQueueComponent(const FObjectInitializer& init) : UActorComponent(init)
{
}

USoundWaveQueueComponent::~USoundWaveQueueComponent()
{
}

void USoundWaveQueueComponent::QueueSound(USoundWave* Sound)
{
	SoundQueue.Enqueue(Sound);
	SoundRefStorage.Add(Sound);
	PlayNextSoundInQueue();
}

void USoundWaveQueueComponent::Stop()
{
	if (bIsPlaying && AudioComponent)
	{
		AudioComponent->Stop();
	}
	bIsPlaying = false;
}

void USoundWaveQueueComponent::ResumePlay()
{
	if (!bIsPlaying && AudioComponent)
	{
		AudioComponent->Play();
		bIsPlaying = true;
	}
}

void USoundWaveQueueComponent::PlayNextSoundInQueue()
{
	if (!bIsPlaying && bAutoPlayOnTarget)
	{
		USoundWave* Sound;
		bool bQueueIsNotEmpty = SoundQueue.Dequeue(Sound);

		//Nothing left in queue - exit
		if (!bQueueIsNotEmpty)
		{
			bIsPlaying = false;
			if (AudioComponent)
			{
				AudioComponent->DestroyComponent();
				AudioComponent = nullptr;
			}
			return;
		}
		float Duration = Sound->Duration;

		//3d in-world playback
		if (Target)
		{
			AudioComponent = UGameplayStatics::SpawnSoundAttached(
				Sound,
				Target,
				FName(),
				FVector(),
				EAttachLocation::SnapToTarget,
				true);

			AudioComponent->AttenuationSettings = AttenuationSettings;

			//now that it's referred elsewhere, remove it from queue storage
			SoundRefStorage.Remove(Sound);

			if (!AudioComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("::PlayNextSoundInQueue AudioComponent startup failed."));
				return;
			}

			AudioComponent->Play();

			// Set the timer with a lambda function
			GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, [this]()
			{
				if (!bIsPlaying)
				{
					return;
				}

				if (AudioComponent)
				{
					AudioComponent->DestroyComponent();
					AudioComponent = nullptr;
				}
				bIsPlaying = false;
				PlayNextSoundInQueue();
			}, Duration, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("::PlayNextSoundInQueue with no Target, not yet implemented."));
		}
		bIsPlaying = true;
	}
}

void USoundWaveQueueComponent::ClearQueue()
{
	SoundQueue.Empty();
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
	Stop();
	Super::EndPlay(EndPlayReason);
}
