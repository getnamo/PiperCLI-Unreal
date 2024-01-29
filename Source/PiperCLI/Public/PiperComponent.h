#pragma once

#include "SubProcess.h"
#include "Components/ActorComponent.h"
#include "PiperComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperBeginProcessSignature, const FString&, StartUpState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperOutputSignature, const FString&, OutputMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPiperEndProcessSignature, const FString&, EndState);

UCLASS(BlueprintType, ClassGroup = "TTS", meta = (BlueprintSpawnableComponent))
class PIPERCLI_API UPiperComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperOutputSignature OnOutput;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperBeginProcessSignature OnBeginProcessing;

	UPROPERTY(BlueprintAssignable, Category = "Piper Events")
	FPiperEndProcessSignature OnEndProcessing;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	FProcessParams PiperCLIParams;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Piper Params")
	bool bStartPiperOnBeginPlay = true;

	//In either json or raw format
	UFUNCTION(BlueprintCallable, Category = "Piper Functions")
	void SendInput(const FString& Text);

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	~UPiperComponent();

protected:
	TSharedPtr<FSubProcessHandler> ProcessHandler;
};