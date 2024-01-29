#include "SubProcess.h"
#include "Async/Async.h"


FSubProcessHandler::FSubProcessHandler()
{

}

FSubProcessHandler::~FSubProcessHandler()
{
	State.bIsBeingDestroyed = true;

	StopProcess();
}

void FSubProcessHandler::StartProcess(const FProcessParams& InParams)
{
	Params = InParams;

	Async(Params.ExecutionContext, [this]
	{
		uint32 OutId;

		void* ReadPipe = nullptr;
		void* WritePipe = nullptr;
		FPlatformProcess::CreatePipe(ReadPipe, WritePipe);
	
		State.ProcessHandle = FPlatformProcess::CreateProc(
			*Params.Url, 
			*Params.Params,
			Params.bLaunchDetached, 
			Params.bLaunchHidden,
			Params.bLaunchReallyHidden,
			&OutId,
			Params.PriorityModifier,
			*Params.OptionalWorkingDirectory,
			WritePipe,
			ReadPipe);

		State.WritePipe = WritePipe;
		State.ReadPipe = ReadPipe;

		State.ProcessId = OutId;
		const int32 LocalId = State.ProcessId;

		if (!Params.InitialStdInput.IsEmpty())
		{
			SendInput(Params.InitialStdInput);
		}

		if (State.ProcessHandle.IsValid()) 
		{
			AsyncTask(ENamedThreads::GameThread, [&, LocalId]
			{
				OnProcessBegin(LocalId, true);
			});
		}
		else
		{
			ClosePipes();

			AsyncTask(ENamedThreads::GameThread, [&, LocalId]
			{
				OnProcessBegin(LocalId, false);
				OnProcessEnd(LocalId, -1);
			});
			return;
		}

		if (Params.bProcessInBytes)
		{
			//Buffer path
			TArray<uint8> Buffer;
			do
			{
				bool bSuccess = FPlatformProcess::ReadPipeToArray(State.ReadPipe, Buffer);

				if (bSuccess)
				{
					if (Params.bOutputToGameThread)
					{
						FThreadSafeBool bEmitComplete;

						//for now gamethread queue the output, Allow non-gamethread returns in future
						AsyncTask(ENamedThreads::GameThread, [&, LocalId]
						{
							OnProcessOutputBytes(LocalId, Buffer);
							bEmitComplete = true;
						});

						while (!bEmitComplete)
						{
							FPlatformProcess::Sleep(0.0001f);
						}
					}
					else
					{
						OnProcessOutputBytes(LocalId, Buffer);
					}
				}
			} while (FPlatformProcess::IsProcRunning(State.ProcessHandle));
		}
		else
		{
			//String path
			FString LatestOutput;
			do
			{
				LatestOutput = FPlatformProcess::ReadPipe(State.ReadPipe);
				State.OutputHistory += LatestOutput;

				if (!LatestOutput.IsEmpty())
				{
					if (Params.bOutputToGameThread)
					{
						const FString SafeLatestOutput = LatestOutput;
						//for now gamethread queue the output, Allow non-gamethread returns in future
						AsyncTask(ENamedThreads::GameThread, [&, LocalId, SafeLatestOutput]
							{
								OnProcessOutput(LocalId, SafeLatestOutput);
							});
					}
					else
					{
						OnProcessOutput(LocalId, LatestOutput);
					}
				}
			} while (FPlatformProcess::IsProcRunning(State.ProcessHandle) || !LatestOutput.IsEmpty());
		}

		int ExitCode = -1;
		bool ReturnCodeValid = FPlatformProcess::GetProcReturnCode(State.ProcessHandle, &ExitCode);

		if (!ReturnCodeValid)
		{
			ExitCode = -1;
		}
		//Only emit if we're not cleaning up
		if (!State.bIsBeingDestroyed)
		{
			AsyncTask(ENamedThreads::GameThread, [this, LocalId, ExitCode]
			{
				OnProcessEnd(LocalId, ExitCode);
			});

		}

		ClosePipes();
	});
}

void FSubProcessHandler::StopProcess()
{
	if (State.ProcessHandle.IsValid())
	{
		//Maybe queue input to bg thread?
		FPlatformProcess::TerminateProc(State.ProcessHandle, false);
		ClosePipes();
	}
}

void FSubProcessHandler::SendInput(const TArray<uint8>& Bytes)
{
	if (State.ProcessHandle.IsValid() && FPlatformProcess::IsProcRunning(State.ProcessHandle))
	{
		FPlatformProcess::WritePipe(State.WritePipe, Bytes.GetData(), Bytes.Num());
	}
}

void FSubProcessHandler::SendInput(const FString& Text)
{
	if (State.ProcessHandle.IsValid() && FPlatformProcess::IsProcRunning(State.ProcessHandle))
	{
		FPlatformProcess::WritePipe(State.WritePipe, Text);
	}
}

void FSubProcessHandler::ClosePipes()
{
	if (State.ReadPipe != nullptr && State.WritePipe != nullptr)
	{
		FPlatformProcess::ClosePipe(State.ReadPipe, State.WritePipe);
		State.ReadPipe = nullptr;
		State.WritePipe = nullptr;
	}
}
