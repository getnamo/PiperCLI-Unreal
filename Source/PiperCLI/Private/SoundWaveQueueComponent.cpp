#include "SoundWaveQueueComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

USoundWaveQueueComponent::USoundWaveQueueComponent(const FObjectInitializer& init) : UActorComponent(init)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
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

void USoundWaveQueueComponent::QueueSound(USoundWave* Sound, const FString& Transcript)
{
	FSWTranscribedSound SoundData;
	SoundData.Sound = Sound;
	SoundData.Transcription = Transcript;

	SoundQueue.Enqueue(SoundData);
	SoundRefStorage.Add(Sound);
	PlayNextSoundInQueue();
}

void USoundWaveQueueComponent::Stop()
{
	if (PlaybackStateInfo.bIsPlaying && AudioComponent)
	{
		AudioComponent->Stop();
	}
	PlaybackStateInfo.bIsPlaying = false;

	//Emit with playback false for tracking purposes
	OnAudioPlayback.Broadcast(PlaybackStateInfo);
}

void USoundWaveQueueComponent::ResumePlay()
{
	if (!PlaybackStateInfo.bIsPlaying && AudioComponent)
	{
		AudioComponent->Play();
		PlaybackStateInfo.bIsPlaying = true;
	}
}

void USoundWaveQueueComponent::PlayNextSoundInQueue()
{
	if (PlaybackStateInfo.bIsPlaying)
	{
		return;
	}

	FSWTranscribedSound SoundData;

	bool bQueueIsNotEmpty = SoundQueue.Dequeue(SoundData);

	USoundWave* Sound = SoundData.Sound;

	//Nothing left in queue - exit
	if (!bQueueIsNotEmpty)
	{
		PlaybackStateInfo.bIsPlaying = false;
		return;
	}

	//Reset playback info
	LastEstimatedSpokenWord = TEXT("");
	PlaybackStateInfo.FullDuration = Sound->Duration;
	PlaybackStateInfo.SectionText = SoundData.Transcription;
	

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

				if(!bUsePlayTimeBasedQueueHandling)
				{
					AudioComponent->OnAudioFinishedNative.AddLambda([this](UAudioComponent* FinishedComponent)
					{
						PlaybackStateInfo.bIsPlaying = false;
						PlayNextSoundInQueue();
					});
					//Capture soundwave procedurals
					AudioComponent->OnAudioPlaybackPercentNative.AddLambda([this](const UAudioComponent* ForAudioComponent, const USoundWave* ForSound, const float PercentDone)
					{
						float Now = GetWorld()->GetTimeSeconds();
						PlaybackStateInfo.PlaybackElapsedSeconds = Now - PlaybackStateInfo.PlayStartTime;
						
						if (!PlaybackStateInfo.SectionText.IsEmpty())
						{
							//this should be close to PercentDone
							float ElapsedFactor = PlaybackStateInfo.PlaybackElapsedSeconds / PlaybackStateInfo.FullDuration;

							//Sync estimated word
							PlaybackStateInfo.EstimatedWord = GetWordAtFactor(ElapsedFactor, PlaybackStateInfo.SectionText);
							LastEstimatedSpokenWord = PlaybackStateInfo.EstimatedWord;
						}

						OnAudioPlayback.Broadcast(PlaybackStateInfo);
						
						if (PercentDone == 1.f)
						{
							AudioComponent->Stop();
							PlaybackStateInfo.bIsPlaying = false;
						}
					});
				}

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
			PlaybackStateInfo.PlayStartTime = GetWorld()->GetTimeSeconds();
			PlaybackStateInfo.bIsPlaying = true;

			//Notify that next sound is playing
			OnNextSoundBeginPlay.Broadcast(Sound);

			//Fallback option that works: wait based
			if (bUsePlayTimeBasedQueueHandling)
			{
				// Set the timer with a lambda function
				GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, [this]()
				{
					PlaybackStateInfo.bIsPlaying = false;
					PlayNextSoundInQueue();
				}, PlaybackStateInfo.FullDuration, false);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("::PlayNextSoundInQueue with no Target, not yet implemented."));
		}
	}
}

bool USoundWaveQueueComponent::IsPlaying()
{
	return PlaybackStateInfo.bIsPlaying;
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

FString USoundWaveQueueComponent::GetWordAtFactor(float Factor, const FString& Text)
{
	// Clamp factor just in case
	Factor = FMath::Clamp(Factor, 0.0f, 1.0f);

	// Total character count
	int32 TotalChars = Text.Len();
	if (TotalChars == 0)
	{
		return TEXT("");
	}

	// Compute character index
	int32 TargetIndex = FMath::FloorToInt(Factor * TotalChars);

	// Split the text into words and track where each word starts and ends
	TArray<FString> Words;
	Text.ParseIntoArrayWS(Words);

	int32 CurrentCharIndex = 0;
	for (const FString& Word : Words)
	{
		// Find this word in the text starting at CurrentCharIndex
		int32 FoundAt = Text.Find(Word, ESearchCase::IgnoreCase, ESearchDir::FromStart, CurrentCharIndex);

		if (FoundAt != INDEX_NONE)
		{
			int32 WordStart = FoundAt;
			int32 WordEnd = WordStart + Word.Len();

			if (TargetIndex >= WordStart && TargetIndex < WordEnd)
			{
				return Word;
			}

			// Advance past this word for next iteration
			CurrentCharIndex = WordEnd;
		}
	}

	// If we didn't find the target in any word (possibly whitespace?), return empty
	return TEXT("");
}

FString USoundWaveQueueComponent::GetTextUpToFactorInclusive(float Factor, const FString& Text){
	// Clamp factor between 0 and 1
	Factor = FMath::Clamp(Factor, 0.0f, 1.0f);

	const int32 TotalChars = Text.Len();
	if (TotalChars == 0)
	{
		return TEXT("");
	}

	// Target character index based on factor
	const int32 TargetIndex = FMath::FloorToInt(Factor * TotalChars);

	// Split into words, keeping whitespace intact so we can rebuild exact positions
	TArray<FString> Words;
	Text.ParseIntoArrayWS(Words);

	int32 CurrentIndex = 0;
	for (const FString& Word : Words)
	{
		// Find this word's actual index in the original text from CurrentIndex onward
		const int32 WordStart = Text.Find(Word, ESearchCase::IgnoreCase, ESearchDir::FromStart, CurrentIndex);

		if (WordStart == INDEX_NONE)
		{
			continue;
		}

		const int32 WordEnd = WordStart + Word.Len();

		// If the target index falls before or inside this word, include up to the end of this word
		if (TargetIndex < WordEnd)
		{
			return Text.Left(WordEnd);
		}

		// Move current index forward for the next word search
		CurrentIndex = WordEnd;
	}

	// If the factor was near 1, include the whole string
	return Text;
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

void USoundWaveQueueComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//do audio progression here if we use time based playback
	if (bUsePlayTimeBasedQueueHandling && PlaybackStateInfo.bIsPlaying)
	{
		float Now = GetWorld()->GetTimeSeconds();
		PlaybackStateInfo.PlaybackElapsedSeconds = Now - PlaybackStateInfo.PlayStartTime;

		//Don't bother emitting if we don't pass in transcripts
		if (!PlaybackStateInfo.SectionText.IsEmpty())
		{
			//this should be close to PercentDone
			float ElapsedFactor = PlaybackStateInfo.PlaybackElapsedSeconds / PlaybackStateInfo.FullDuration;
			PlaybackStateInfo.PlaybackFactor = ElapsedFactor;

			//Sync estimated word
			PlaybackStateInfo.EstimatedWord = GetWordAtFactor(ElapsedFactor, PlaybackStateInfo.SectionText);
			if (PlaybackStateInfo.EstimatedWord != LastEstimatedSpokenWord)
			{
				OnEstimatedWordSpoken.Broadcast(PlaybackStateInfo);
			}
			LastEstimatedSpokenWord = PlaybackStateInfo.EstimatedWord;
		}

		OnAudioPlayback.Broadcast(PlaybackStateInfo);
	}
}
