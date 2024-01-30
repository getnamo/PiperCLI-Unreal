#pragma once

#include "SubProcess.h"
#include "Components/ActorComponent.h"
#include "PiperComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperBeginProcessSignature, const FString&, StartUpState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperOutputSignature, const FString&, OutputMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperOutputBinarySignature, const TArray<uint8>&, Buffer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperEndProcessSignature, const FString&, EndState);


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
};

UCLASS(BlueprintType, ClassGroup = "TTS", meta = (BlueprintSpawnableComponent))
class PIPERCLI_API UPiperComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperOutputSignature OnOutput;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperOutputBinarySignature OnOutputBytes;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperEndProcessSignature OnEndProcessing;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperBeginProcessSignature OnBeginProcessing;

	//Specify voice model
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	FPiperParams PiperParams;

	//Some of these may be overwritten by options in PiperParams
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	FProcessParams CLIParams;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bStartPiperOnBeginPlay = true;

	//If input is sent and process isn't running, start it
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bLazyAutoStartProcess = true;

	UPROPERTY(BlueprintReadOnly, Category = "Piper Params")
	bool bPiperProcessRunning = false;

	//In either json or raw format
	UFUNCTION(BlueprintCallable, Category = "Piper Functions")
	void SendInput(const FString& Text);

	UFUNCTION(BlueprintCallable, Category = "Piper Functions")
	void StartPiperProcess();

	UFUNCTION(BlueprintCallable, Category = "Piper Functions")
	void StopPiperProcess();

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SyncCLIParams();

	~UPiperComponent();

protected:
	TSharedPtr<FSubProcessHandler> ProcessHandler;
};