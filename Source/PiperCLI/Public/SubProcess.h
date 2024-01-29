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
	int32 PriorityModifier;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	FString OptionalWorkingDirectory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	bool bOutputToGameThread = true;

	EAsyncExecution ExecutionContext = EAsyncExecution::Thread;
};

USTRUCT(BlueprintType)
struct FProcessState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessState")
	int32 ProcessId = -1;

	//These are not blueprint accessible
	FProcHandle ProcessHandle;
	void* WritePipe;
	void* ReadPipe;
	bool bIsBeingDestroyed = false;
};


class FSubProcessHandler
{
public:

	FSubProcessHandler();
	~FSubProcessHandler();

	TFunction<void(const int32 ProcessId, bool StartSucceded)> OnProcessBegin = nullptr;
	TFunction<void(const int32 ProcessId, const FString& OutputString)> OnProcessOutput = nullptr;
	TFunction<void(const int32 ProcessId, int32 ReturnCode)> OnProcessEnd = nullptr;

	void StartProcess(const FProcessParams& InParams);

	void StopProcess();

	FProcessParams Params;

protected:
	FProcessState State;
};