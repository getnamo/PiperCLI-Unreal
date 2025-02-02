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

	//If true, the component will auto-convert pcm bytes to USoundWaveProcedural
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bOutputSoundWaves = true;

	//if true it will output to bytes. Turn off for efficiency boost
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bOutputBytes = false;

	//Default for medium model
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	int32 SampleRate = 22050;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	int32 Channels = 1;

	//if different than sample rate it will be re-sampled internally before being emitted (only for byte/chunk outputs not soundwaves).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	int32 ByteOutputSampleRate = 22050;

	//Set to positive integer if you wish to stream the output in chunks for e.g. downstream systems. If -1 it will not chunk. Only affects byte output.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	int32 OutputChunkSize = -1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperOnGeneratedAudioSignature, USoundWave*, GeneratedSound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPiperOutputChunkSignature, const TArray<uint8>&, Buffer, int32, ChunkIndex, int32, TotalChunks);

UCLASS(BlueprintType, ClassGroup = "CLI", meta = (BlueprintSpawnableComponent))
class PIPERCLI_API UPiperComponent : public UCLIProcessComponent
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperOnGeneratedAudioSignature OnAudioGenerated;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperOutputChunkSignature OnChunkGenerated;

	//Specify voice model
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	FPiperParams PiperParams;

	UFUNCTION(BlueprintCallable, Category = "Audio Utility")
	TArray<uint8> PCMToWav(const TArray<uint8>& InPCMBytes, int32 SampleRate = 16000, int32 Channels = 1);

	UFUNCTION(BlueprintCallable, Category = "Audio Utility")
	USoundWave* WavToSoundWave(const TArray<uint8>& InWavBytes);

	UFUNCTION(BlueprintCallable, Category = "Audio Utility")
	void ResamplePCM(const TArray<uint8>& SourcePCM, int32 SourceSampleRate, int32 TargetSampleRate, TArray<uint8>& OutResampledPCM);


	void ChunkByteArray(const TArray<uint8>& InputArray, int32 ChunkSize, TArray<TArray<uint8>>& OutChunks);

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