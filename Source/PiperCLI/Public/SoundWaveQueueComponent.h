#pragma once

#include "Components/ActorComponent.h"
#include "Sound/SoundWave.h"
#include "SoundWaveQueueComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoundWaveSignature, USoundWave*, Sound);

UCLASS(BlueprintType, ClassGroup = "TTS", meta = (BlueprintSpawnableComponent))
class PIPERCLI_API USoundWaveQueueComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintAssignable, Category = "SoundWaveQueue Events")
	FSoundWaveSignature OnNext;

	//Ideally we'd use an on-finished callback
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Properties")
	bool bUsePlayTimeBasedQueueHandling = true;

	//If false, handle your own playback logic
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Properties")
	bool bAutoPlayOnTarget = true;

	//Target of our sound source. If null, will play as 2D UI sound
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Properties")
	USceneComponent* Target;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoundWaveQueue Properties")
	USoundAttenuation* AttenuationSettings;

	UPROPERTY(BlueprintReadOnly, Category = "SoundWaveQueue Properties")
	bool bIsPlaying;

	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void QueueSound(USoundWave* Sound);

	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void PlayNextSoundInQueue();

	//Stop the current audio playback e.g. an interrupt
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void Stop();

	//If current audio stopped, this will resume the play again
	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void ResumePlay();

	UFUNCTION(BlueprintCallable, Category = "SoundWaveQueue Functions")
	void ClearQueue();

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	~USoundWaveQueueComponent();

protected:
	FTimerHandle DelayTimerHandle;
	UAudioComponent* AudioComponent;

	UPROPERTY()
	TSet<USoundWave*> SoundRefStorage;

	TQueue<USoundWave*> SoundQueue;
};