#pragma once

#include "Components/ActorComponent.h"
#include "Sound/SoundWave.h"
#include "FIFOQueue.h"
#include "SoundWaveQueueComponent.generated.h"

USTRUCT(BlueprintType)
struct FSWQSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Settings")
	USoundAttenuation* AttenuationSettings;

	//Ideally we'd use an on-finished callback
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Settings")
	bool bUsePlayTimeBasedQueueHandling = true;

	//If false, handle your own playback logic
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Settings")
	bool bAutoPlayOnTarget = true;

	//has performance implications
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Settings")
	bool bStreamEmitBytesIfAvailable = false;

	//on the first byte stream, look ahead by this amount and stream it immediately
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Settings")
	int32 BeginByteStreamLookaheadChunk = 0;
};

//Wrapped data structure to allow easier downstream parsing
USTRUCT(BlueprintType)
struct FSWSoundData
{
	GENERATED_USTRUCT_BODY()

	//Wrapped Soundwave for playback
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sound Data")
	USoundWave* SoundWave;

	//Optional extra data used for matching emits

	//Matching transcript
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sound Data")
	FString Transcript;

	//Duration of generated clip
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sound Data")
	float Duration = 0.f;

	//Duplicated - raw bytes in the desired format, will be empty if piper bOutputBytes is false
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sound Data")
	TArray<uint8> PCMBytes;

	//if sample rate is -1, we haven't set out byte output
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	int32 ByteOutputSampleRate = -1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoundWaveSignature, USoundWave*, Sound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoundDataSignature, const FSWSoundData&, SoundData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoundChunkSignature, const TArray<uint8>&, SoundChunk);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAudioComponentSignature, UAudioComponent*, AudioComponent);

USTRUCT(BlueprintType)
struct FSWPlayStateInformation
{
	GENERATED_USTRUCT_BODY()

	/* Lerp based on time, won't be 100% aligned. Can be used for rough sub-titling */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FSWPlayStateInformation Properties")
	FString EstimatedWord;

	/** Sentence or full text sent with Sound*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FSWPlayStateInformation Properties")
	FString SectionText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FSWPlayStateInformation Properties")
	float PlaybackFactor = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FSWPlayStateInformation Properties")
	float PlaybackElapsedSeconds = 0.f;

	//full duration of sound being played
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FSWPlayStateInformation Properties")
	float FullDuration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FSWPlayStateInformation Properties")
	float PlayStartTime = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FSWPlayStateInformation Properties")
	bool bIsPlaying = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FSWPlayStateInformation Properties")
	int32 SampleRate = 16000;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlaybackStateSignature, const FSWPlayStateInformation&, PlaybackState);

UCLASS(BlueprintType, ClassGroup = "TTS", meta = (BlueprintSpawnableComponent))
class PIPERCLI_API USoundWaveQueueComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:

	//Called when the next audio is being played, can be used to interrupt or do additional processing if needed
	UPROPERTY(BlueprintAssignable, Category = "SoundWaveQueue Events")
	FSoundDataSignature OnNextSoundBeginPlay;

	UPROPERTY(BlueprintAssignable, Category = "SoundWaveQueue Events")
	FAudioComponentSignature OnAudioComponentCreated;

	//called every frame when playing for downstream handling
	UPROPERTY(BlueprintAssignable, Category = "SoundWaveQueue Events")
	FPlaybackStateSignature OnAudioPlayback;

	//If you pass in transcript info, you can callbacks when the word should be spoken
	UPROPERTY(BlueprintAssignable, Category = "SoundWaveQueue Events")
	FPlaybackStateSignature OnEstimatedWordSpoken;

	//roughly synced sound chunk
	UPROPERTY(BlueprintAssignable, Category = "SoundWaveQueue Events")
	FSoundChunkSignature OnSyncedSoundChunk;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Properties")
	FSWQSettings Settings;

	//Target of our sound source. If null, will play as 2D UI sound
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Properties")
	USceneComponent* Target;

	UPROPERTY(BlueprintReadOnly, Category = "SoundWaveQueue Properties")
	UAudioComponent* AudioComponent;

	UPROPERTY(BlueprintReadOnly, Category = "SoundWaveQueue Properties")
	FSWPlayStateInformation PlaybackStateInfo;

	//In seconds of audio remaining
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	float GetQueueDepthSeconds();

	//queue depth in number of remaining soundwaves
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	int32 GetQueueDepth();

	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void QueueSound(USoundWave* Sound, const FString& Transcript = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void QueueSoundData(const FSWSoundData& SoundData);

	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void PlayNextSoundInQueue();

	UFUNCTION(BlueprintPure, Category = "SoundWaveQueue Functions")
	bool IsPlaying();

	//Stop the current audio playback e.g. an interrupt
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void Stop();

	//If current audio stopped, this will resume the play again
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void ResumePlay();

	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void ClearQueue();

	//Call this after you change the target scene component
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void DestroyTargetAudioComponent();

	//Finds nearest word to given factor
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Utility")
	FString GetWordAtFactor(float Factor, const FString& Text);

	//useful to get the text that was spoken up until cutoff portion
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Utility")
	FString GetTextUpToFactorInclusive(float Factor, const FString& Text);
	
	//utility function for getting text between these two calls
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Utility")
	void StartTrackingText();

	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Utility")
	FString StopTrackingText();

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	~USoundWaveQueueComponent();

protected:

	UPROPERTY()
	TSet<USoundWave*> SoundRefStorage;

	TQueue<FSWSoundData> SoundQueue;

	FByteFIFOQueue SoundChunkQueue;

	FString LastEstimatedSpokenWord;

	UPROPERTY()
	FString TrackedText;

	bool bIsTrackingText = false;

	FTimerHandle DelayTimerHandle;
};