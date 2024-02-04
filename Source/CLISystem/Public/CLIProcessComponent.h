#pragma once

#include "SubProcess.h"
#include "Components/ActorComponent.h"
#include "CLIProcessComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCLIBeginProcessSignature, const FString&, StartUpState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCLIEndProcessSignature, const FString&, EndState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCLIOutputStringSignature, const FString&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCLIOutputBinarySignature, const TArray<uint8>&, Buffer);



UCLASS(BlueprintType, ClassGroup = "TTS", meta = (BlueprintSpawnableComponent))
class CLISYSTEM_API UCLIProcessComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FCLIBeginProcessSignature OnBeginProcessing;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FCLIEndProcessSignature OnEndProcessing;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FCLIOutputStringSignature OnOutputText;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FCLIOutputBinarySignature OnOutputBytes;

	//Some of these may be overwritten by options in PiperParams
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	FProcessParams CLIParams;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bStartProcessOnBeginPlay = true;

	//If input is sent and process isn't running, start it
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bLazyAutoStartProcess = true;

	UPROPERTY(BlueprintReadOnly, Category = "Piper Params")
	bool bProcessIsRunning = false;

	//In either json or raw format
	UFUNCTION(BlueprintCallable, Category = "Piper Functions")
	virtual void SendInput(const FString& Text);

	UFUNCTION(BlueprintCallable, Category = "Piper Functions")
	virtual void StartProcess();

	UFUNCTION(BlueprintCallable, Category = "Piper Functions")
	virtual void StopProcess();

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	~UCLIProcessComponent();

protected:
	TSharedPtr<FSubProcessHandler> ProcessHandler;
};