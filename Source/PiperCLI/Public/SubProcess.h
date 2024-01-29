#pragma once
#include "GenericPlatform/GenericPlatformProcess.h"
#include "SubProcess.generated.h"


USTRUCT(BlueprintType)
struct FProcessParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FProcessParams")
	FString Url;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	FString Params;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	bool bLaunchDetached;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	bool bLaunchHidden;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	bool bLaunchReallyHidden;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	int32 ProcessId = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	int32 PriorityModifier;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	FString OptionalWorkingDirectory;
};


class FSubProcessHandler
{
public:

	FSubProcessHandler();
	~FSubProcessHandler();

	TFunction<void(const FString& ProcessId, bool StartSucceded)> OnProcessBegin;
	TFunction<void(const FString& ProcessId, const FString& OutputString)> OnProcessOutput;
	TFunction<void(const FString& ProcessId, int32 ReturnResult)> OnProcessEnd;

	void StartProcess(const FProcessParams& InParams);

	void StopProcess();

	FProcessParams Params;

private:

	FProcHandle ProcessHandle;
};