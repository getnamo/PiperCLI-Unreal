#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SoundWaveQueueComponent.h"
#include "HAL/PlatformTime.h"
#include "PiperCLITestReceiver.generated.h"

/** Collects UPiperComponent events for the automation tests (game thread). */
UCLASS()
class UPiperCLITestReceiver : public UObject
{
	GENERATED_BODY()
public:
	TArray<FSWSoundData> Sounds;
	TArray<TArray<uint8>> ByteOutputs;
	TArray<FString> Texts;
	int32 Begins = 0;
	int32 Ends = 0;
	double LastAudioTime = 0.0;

	UFUNCTION()
	void OnAudio(const FSWSoundData& Data)
	{
		Sounds.Add(Data);
		LastAudioTime = FPlatformTime::Seconds();
	}

	UFUNCTION()
	void OnBytes(const TArray<uint8>& Bytes) { ByteOutputs.Add(Bytes); }

	UFUNCTION()
	void OnText(const FString& Text) { Texts.Add(Text); }

	UFUNCTION()
	void OnBegin(const FString& State) { ++Begins; }

	UFUNCTION()
	void OnEnd(const FString& State) { ++Ends; }

	int32 NumEvents() const { return Sounds.Num() + ByteOutputs.Num() + Texts.Num() + Begins + Ends; }
};
