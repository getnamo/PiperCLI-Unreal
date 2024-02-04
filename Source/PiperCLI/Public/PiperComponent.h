#pragma once

#include "CLIProcessComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "PiperComponent.generated.h"

USTRUCT(BlueprintType)
struct FPiperParams
{
	GENERATED_USTRUCT_BODY()

	//relative to 'ThirdParty/Piper/Win64/model'
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	FString VoiceModelName = TEXT("model.onnx");

	//If true, expects text input in e.g. {"text":"hello!"}
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bUseJsonFormatInput = false;

	//If false, you will generate text files instead in piper directory
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bOutputToUnreal = true;

	//Set false if you want to specify CLI params fully yourself
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bSyncCLIParams = true;

	//If true, the component will auto-convert pcm bytes to USoundWaveProcedural and not emit to OnOutputBytes
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bOutputSoundWaves = true;

	//Default for medium model
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	int32 SampleRate = 22050;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	int32 Channels = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperOnGeneratedAudioSignature, USoundWave*, GeneratedSound);

UCLASS(BlueprintType, ClassGroup = "CLI", meta = (BlueprintSpawnableComponent))
class PIPERCLI_API UPiperComponent : public UCLIProcessComponent
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperOnGeneratedAudioSignature OnAudioGenerated;

	//Specify voice model
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	FPiperParams PiperParams;

	UFUNCTION(BlueprintCallable, Category = "Audio Utility")
	TArray<uint8> PCMToWav(const TArray<uint8>& InPCMBytes, int32 SampleRate = 16000, int32 Channels = 1);

	UFUNCTION(BlueprintCallable, Category = "Audio Utility")
	USoundWave* WavToSoundWave(const TArray<uint8>& InWavBytes);

	//UCLIProcessComponent overrides
	virtual void StartProcess() override;

	//UActorComponent overrides
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SyncCLIParams();

	~UPiperComponent();

protected:

	void SetSoundWaveFromWavBytes(USoundWaveProcedural* InSoundWave, const TArray<uint8>& InBytes);
};