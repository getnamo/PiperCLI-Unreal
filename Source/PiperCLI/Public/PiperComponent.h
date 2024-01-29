#pragma once

#include "SubProcess.h"
#include "Components/ActorComponent.h"
#include "PiperComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = "TTS", meta = (BlueprintSpawnableComponent))
class PIPERCLI_API UPiperComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;

	~UPiperComponent();

protected:
	TSharedPtr<FSubProcessHandler> ProcessHandler;
};