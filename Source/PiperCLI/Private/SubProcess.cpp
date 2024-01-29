#include "SubProcess.h"


FSubProcessHandler::FSubProcessHandler()
{

}

FSubProcessHandler::~FSubProcessHandler()
{

}

void FSubProcessHandler::StartProcess(const FProcessParams& InParams)
{
	Params = InParams;
	uint32 OutId;
	void* PipeWriteChild;
	void* PipeReadChild;

	ProcessHandle = FPlatformProcess::CreateProc(
		*Params.Url, 
		*Params.Params,
		Params.bLaunchDetached, 
		Params.bLaunchHidden,
		Params.bLaunchReallyHidden,
		&OutId,
		Params.PriorityModifier,
		*Params.OptionalWorkingDirectory,
		PipeWriteChild,
		PipeReadChild);
}

void FSubProcessHandler::StopProcess()
{
	if (ProcessHandle.IsValid())
	{
		FPlatformProcess::TerminateProc(ProcessHandle, false);
		//FPlatformProcess::Close
	}
	

	//Signal stop1

	//Wait
	/*FProcState& State = ProcessHandler.GetProcessInfo();
	if (State.bRunning())
	{
		State.GetProcessInfo().Wait();
		State = ProcessHandler.GetProcessInfo();
	}
	
	int32 ReturnCode;
	bool bSuccess = State.GetReturnCode(ReturnCode);*/
}
