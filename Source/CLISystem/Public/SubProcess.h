#pragma once
#include "GenericPlatform/GenericPlatformProcess.h"
#include "SubProcess.generated.h"


USTRUCT(BlueprintType)
struct CLISYSTEM_API FProcessParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FProcessParams")
	FString Url;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	FString Params;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	FString InitialStdInput;

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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	bool bProcessInBytes = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessParams")
	bool bWaitForSend = false;

	EAsyncExecution ExecutionContext = EAsyncExecution::Thread;
};

USTRUCT(BlueprintType)
struct CLISYSTEM_API FProcessState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessState")
	int32 ProcessId = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FProcessState")
	FString OutputHistory;

	//These are not blueprint accessible
	FProcHandle ProcessHandle;
	void* OutputPipeWrite;
	void* OutputPipeRead;
	void* InputPipeWrite;
	void* InputPipeRead;
	bool bIsBeingDestroyed = false;
};


class CLISYSTEM_API FSubProcessHandler
{
public:

	FSubProcessHandler();
	~FSubProcessHandler();

	TFunction<void(const int32 ProcessId, bool StartSucceded)> OnProcessBegin = nullptr;
	TFunction<void(const int32 ProcessId, const FString& OutputString)> OnProcessOutput = nullptr;
	TFunction<void(const int32 ProcessId, const TArray<uint8>& OutputBytes)> OnProcessOutputBytes = nullptr;
	TFunction<void(const int32 ProcessId, int32 ReturnCode)> OnProcessEnd = nullptr;

	void StartProcess(const FProcessParams& InParams);
	void StopProcess();

	void SendInput(const TArray<uint8>& Bytes);
	void SendInput(const FString& Text);

	FProcessParams Params;

protected:

	void ClosePipes();
	FProcessState State;

	FThreadSafeBool WaitLockActive;
};