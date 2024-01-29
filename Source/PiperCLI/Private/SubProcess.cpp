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

		/*AsyncTask(ENamedThreads::GameThread, []
		{
		});*/
	
		
		uint32 OutId;

		void* ReadPipe = nullptr;
		void* WritePipe = nullptr;
		FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

		FString Output;
	
		State.ProcessHandle = FPlatformProcess::CreateProc(
			*Params.Url, 
			*Params.Params,
			Params.bLaunchDetached, 
			Params.bLaunchHidden,
			Params.bLaunchReallyHidden,
			&OutId,
			Params.PriorityModifier,
			nullptr,//*Params.OptionalWorkingDirectory,
			WritePipe,
			ReadPipe,
			WritePipe);

		State.WritePipe = WritePipe;
		State.ReadPipe = ReadPipe;

		State.ProcessId = OutId;
		const int32 LocalId = State.ProcessId;

		if (State.ProcessHandle.IsValid()) 
		{
			AsyncTask(ENamedThreads::GameThread, [&, LocalId]
			{
				OnProcessBegin(LocalId, true);
			});
		}
		else
		{
			FPlatformProcess::ClosePipe(State.ReadPipe, State.WritePipe);
			AsyncTask(ENamedThreads::GameThread, [&, LocalId]
			{
				OnProcessBegin(LocalId, false);
				OnProcessEnd(LocalId, -1);
			});
			return;
		}

		FString LatestOutput;

		do 
		{
			LatestOutput = FPlatformProcess::ReadPipe(ReadPipe);
			Output += LatestOutput;

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

		}});

}

void FSubProcessHandler::StopProcess()
{
	if (State.ProcessHandle.IsValid())
	{
		//Maybe queue input to bg thread?
		FPlatformProcess::ClosePipe(State.ReadPipe, State.WritePipe);
		FPlatformProcess::TerminateProc(State.ProcessHandle, false);
	}
}
