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

float USoundWaveQueueComponent::GetQueueDepthSeconds()
{
	float Duration = 0.f;
	for (auto Sound : SoundRefStorage)
	{
		Duration += Sound->GetDuration();
	}
	return Duration;
}

int32 USoundWaveQueueComponent::GetQueueDepth()
{
	return SoundRefStorage.Num();
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
	if (bIsPlaying)
	{
		return;
	}

	USoundWave* Sound;
	bool bQueueIsNotEmpty = SoundQueue.Dequeue(Sound);

	//Nothing left in queue - exit
	if (!bQueueIsNotEmpty)
	{
		bIsPlaying = false;
		return;
	}

	float Duration = Sound->Duration;

	if (bAutoPlayOnTarget)
	{
		//3d in-world playback
		if (Target)
		{
			if(!AudioComponent)
			{
				AudioComponent = UGameplayStatics::SpawnSoundAttached(
					Sound,
					Target,
					FName(),
					FVector(),
					EAttachLocation::SnapToTarget,
					true);

				AudioComponent->OnAudioFinishedNative.AddLambda([this](UAudioComponent* FinishedComponent)
				{
					bIsPlaying = false;
					PlayNextSoundInQueue();
				});

				//Capture soundwave procedurals
				AudioComponent->OnAudioPlaybackPercentNative.AddLambda([this](const UAudioComponent* ForAudioComponent, const USoundWave* ForSound, const float PercentDone)
				{
					if (PercentDone == 1.f)
					{
						AudioComponent->Stop();
						bIsPlaying = false;
					}
				});

				OnAudioComponentCreated.Broadcast(AudioComponent);
			}
			else
			{
				AudioComponent->SetSound(Sound);
			}

			AudioComponent->AttenuationSettings = AttenuationSettings;

			//now that it's referred elsewhere, remove it from queue storage
			SoundRefStorage.Remove(Sound);

			if (!AudioComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("::PlayNextSoundInQueue AudioComponent startup failed."));
				return;
			}

			AudioComponent->Play();

			bIsPlaying = true;

			//Notify that next sound is playing
			OnNextSoundBeginPlay.Broadcast(Sound);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("::PlayNextSoundInQueue with no Target, not yet implemented."));
		}
	}
	else
	{
		bIsPlaying = true;
	}
}

void USoundWaveQueueComponent::ClearQueue()
{
	SoundQueue.Empty();
}

void USoundWaveQueueComponent::DestroyTargetAudioComponent()
{
	if (AudioComponent)
	{
		AudioComponent->DestroyComponent();
		AudioComponent = nullptr;
	}
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

	DestroyTargetAudioComponent();
	Super::EndPlay(EndPlayReason);
}
