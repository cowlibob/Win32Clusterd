#include <Windows.h>
#include <time.h>
#include "DebugLog.h"

struct ProcessDetail
{
	ProcessDetail(unsigned int port)
	{
		exited = false;
		pi = new PROCESS_INFORMATION;
		ZeroMemory(pi, sizeof(*pi));

		this->port = port;

		_time64(&started);
	}
	
	~ProcessDetail()
	{
		if(!exited)
		{
			DebugLog log;
			log.log(TEXT("Asking process %d to quit"), pi->dwProcessId);
			// Kind of harsh, but to use
			//		GenerateConsoleCtrlEvent(CTRL_C_EVENT, it->pi->dwProcessId);
			// we would need to start a new parent process to send
			// the CTRL_C_EVENT to a new process group it heads.
			// So we just terminate child processes.
			TerminateProcess(pi->hProcess, 0);
		}
		CloseHandle(pi->hProcess);
		CloseHandle(pi->hThread);

		ZeroMemory(pi, sizeof(*pi));
		delete pi;
	}

	PROCESS_INFORMATION		*pi;			// CreateProcess info
	unsigned int			port;			// Associated port number
	__time64_t				started;		// Timestamp
	DWORD					exit_code;		// Process Exit code
	bool					exited;			// Discarded flag
};