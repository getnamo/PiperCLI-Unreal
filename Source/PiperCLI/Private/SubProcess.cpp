#include "SubProcess.h"
#include "Async/Async.h"


FSubProcessHandler::FSubProcessHandler()
{

}

FSubProcessHandler::~FSubProcessHandler()
{
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
		FPlatformProcess::CreatePipe(WritePipe, ReadPipe);

		FString Output;
	
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

		if (State.ProcessHandle.IsValid()) 
		{
			AsyncTask(ENamedThreads::GameThread, [&, LocalId]
			{
				OnProcessBegin(LocalId, true);
			});
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread, [&, LocalId]
			{
				OnProcessBegin(LocalId, false);
				OnProcessEnd(LocalId, -1);
			});
			return;
		}

		

		FString LatestOutput = FPlatformProcess::ReadPipe(ReadPipe);
		while (FPlatformProcess::IsProcRunning(State.ProcessHandle) || !LatestOutput.IsEmpty())
		{
			Output += LatestOutput;
			LatestOutput = FPlatformProcess::ReadPipe(ReadPipe);

			if (!LatestOutput.IsEmpty())
			{
				const FString SafeLatestOutput = LatestOutput;

				//for now gamethread queue the output, Allow non-gamethread returns in future
				AsyncTask(ENamedThreads::GameThread, [&, LocalId, SafeLatestOutput]
				{
					OnProcessOutput(LocalId, SafeLatestOutput);
				});
			}
		}

		int ExitCode = -1;
		bool ReturnCodeValid = FPlatformProcess::GetProcReturnCode(State.ProcessHandle, &ExitCode);

		if (!ReturnCodeValid)
		{
			ExitCode = -1;
		}
		AsyncTask(ENamedThreads::GameThread, [this, LocalId, ExitCode]
		{
			OnProcessEnd(LocalId, ExitCode);
		});
	});

}

void FSubProcessHandler::StopProcess()
{
	if (State.ProcessHandle.IsValid())
	{
		//Maybe queue input to bg thread?
		FPlatformProcess::TerminateProc(State.ProcessHandle, false);
		FPlatformProcess::ClosePipe(State.WritePipe, State.ReadPipe);
	}
}
