// main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include "Win32Clusterd.h"
#include "Win32ClusterdConfig.h"
#include "help.h"

#define SERVICE_NAME_W L"Win32Clusterd"
#define SERVICE_NAME "Win32Clusterd"

#ifdef _DEBUG
#	define DEBUG_ONLY(x) x
#	define log(_x) OutputDebugStringA(_x)
//#	define SERVICE_WAIT_FOR_DEBUGGER
#else
#	define DEBUG_ONLY(x)
#	define log(_x)
#endif

SERVICE_STATUS			ServiceStatus;
SERVICE_STATUS_HANDLE	hStatus;
SC_HANDLE				g_schSCManager;
Win32Clusterd			*cluster = NULL;
TCHAR					*g_serviceName = NULL;
Win32ClusterdConfig		*g_config = NULL;

void ServiceMain(int argc, char** argv);
void ControlHandler(DWORD request);
int InitService();

void run_loop()
{
	cluster = new Win32Clusterd(g_config);
	if(!cluster->is_initialized())
	{
		log("Failed to execute because some configuration data is missing.");
		return;
	}

	log("Win32 Cluster Service started.");
	bool restart_now = false;
	while(ServiceStatus.dwCurrentState == SERVICE_RUNNING && !cluster->should_quit())
	{
		cluster->update();
	}
	
	// Child processes will automatically be sent SIGHUP as we close.
	// We may need to implement a SIGHUP trap in the child process to cleanup and exit.
	// A ruby example follows.
	//
	//		Signal.trap("HUP") {
	//			exit(true)
	//		}
	//
	delete cluster;
	log("Win32 Cluster Service shutting down.");
}

int run_as_service()
{
	log("Win32 Cluster - Starting as Service");
	SERVICE_TABLE_ENTRY ServiceTable[2];
	ServiceTable[0].lpServiceName = g_serviceName;
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;

	// Start the control dispatcher thread
	StartServiceCtrlDispatcher(ServiceTable);
	return 0;
}

int run_as_app()
{
	log("Win32 Cluster - Starting as Application");
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	run_loop();
	return 0;
}

SC_HANDLE open_service(DWORD desired_access)
{
    SC_HANDLE schService;

	g_schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if (NULL == g_schSCManager) 
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return NULL;
    }

    // Get a handle to the service.

    schService = OpenService( 
        g_schSCManager,			// SCM database 
        g_serviceName,			// name of service 
        desired_access);

    if (schService == NULL)
    { 
        printf("OpenService failed (%d)\n", GetLastError()); 
        CloseServiceHandle(g_schSCManager);
    }

	return schService;
}

void close_service(SC_HANDLE schService)
{
	CloseServiceHandle(schService); 
    CloseServiceHandle(g_schSCManager);
}

int install_service()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH * 2];

	if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return 1;
	}

	// Add command line to enforce running as a service
	_tcscat_s(szPath, SIZEOF_CHARS(szPath), TEXT(" service"));

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 
 
	if (NULL == schSCManager) 
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return 1;
	}

	// Create the service
	schService = CreateService( 
		schSCManager,				// SCM database 
		g_serviceName,				// name of service 
		g_serviceName,				// service name to display 
		SERVICE_ALL_ACCESS,			// desired access 
		SERVICE_WIN32_OWN_PROCESS,	// service type 
		SERVICE_AUTO_START,			// start type 
		SERVICE_ERROR_NORMAL,		// error control type 
		szPath,						// path to service's binary 
		NULL,						// no load ordering group 
		NULL,						// no tag identifier 
		NULL,						// no dependencies 
		NULL,						// LocalSystem account 
		NULL);						// no password 
 
	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return 1;
	}
	else
		printf("Service installed successfully\n"); 

	// Set the description that is visible in the administration tools server list
	SERVICE_DESCRIPTION sd;
	sd.lpDescription = (TCHAR*)TEXT("Manages multiple ruby processes.");
	BOOL status = ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd);
	_ASSERT(status != 0);

	// Ensure that the service restarts in the event of any problems
	SERVICE_FAILURE_ACTIONS	sfa;
	static SC_ACTION failureActions[] = 
	{
		SC_ACTION_NONE
	};
	sfa.dwResetPeriod = INFINITE;
	sfa.lpRebootMsg = NULL;
	sfa.lpCommand = NULL;
	sfa.cActions = sizeof(failureActions) / sizeof(failureActions[0]);
	sfa.lpsaActions = failureActions;
	status = ChangeServiceConfig2(schService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);
	_ASSERT(status != 0);
	
	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);	
	return 0;
}

int uninstall_service()
{
	int failed = TRUE;
    SC_HANDLE schService = open_service(DELETE);
 
    if (schService)
    { 
		// Delete the service.
 
		if (! DeleteService(schService) ) 
		{
			printf("DeleteService failed (%d)\n", GetLastError()); 
		}
		else
		{
			printf("Service unregistered successfully\n"); 
			failed = FALSE;
		}
	    close_service(schService);
    }

	return failed;
}

int start_service()
{
	int failed = TRUE;
	SC_HANDLE schService = open_service(SERVICE_START);
	if(schService)
	{
		if(!StartService(schService, 0, NULL))
		{
			printf("Service could not be started\n");
		}
		else
		{
			printf("Service was started.\n");
			failed = FALSE;
		}
		close_service(schService);
	}

	return failed;
}

int output_service_state(DWORD current_state)
{
	int failed = TRUE;
	switch(current_state)
	{
	case SERVICE_STOP_PENDING:
		printf("Service is now stopping.\n");
		failed = FALSE;
		break;
	case SERVICE_STOPPED:
		printf("Service is stopped.\n");
		failed = FALSE;
		break;
	}
	return failed;
}

int stop_service()
{
	int failed = TRUE;
	SC_HANDLE schService = open_service(SERVICE_STOP);
	if(schService)
	{
		SERVICE_STATUS status;
		switch(ControlService(schService, SERVICE_CONTROL_STOP, &status))
		{
		case NO_ERROR:
			failed = output_service_state(status.dwCurrentState);
			break;
		case ERROR_SERVICE_NOT_ACTIVE:
			failed = output_service_state(status.dwCurrentState);
			break;
		case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
			printf("Service could not be stopped:\n\t");
			failed = output_service_state(status.dwCurrentState);
			break;
		case ERROR_INVALID_SERVICE_CONTROL:
			printf("Service could not be controlled:\n\t");
			output_service_state(status.dwCurrentState);
			failed = TRUE;
			break;
		}
	}
	return failed;
}

int help(int argc, _TCHAR* argv[])
{
	
	switch(argc)
	{
	case 1:
	case 2:
		_tprintf(HELP_GENERAL);
		break;
	case 3:
		if(_tcsicmp(argv[2], TEXT("commands")) == 0)
			_tprintf(HELP_COMMANDS);
		else if(_tcsicmp(argv[2], TEXT("configure")) == 0)
			_tprintf(HELP_CONFIGURE);
		else if(_tcsicmp(argv[2], TEXT("clean")) == 0)
			_tprintf(HELP_CLEAN);
		else if(_tcsicmp(argv[2], TEXT("install")) == 0)
			_tprintf(HELP_INSTALL);
		else if(_tcsicmp(argv[2], TEXT("uninstall")) == 0)
			_tprintf(HELP_UNINSTALL);
		else if(_tcsicmp(argv[2], TEXT("start")) == 0)
			_tprintf(HELP_START);
		else if(_tcsicmp(argv[2], TEXT("stop")) == 0)
			_tprintf(HELP_STOP);
		else if(_tcsicmp(argv[2], TEXT("run")) == 0)
			_tprintf(HELP_RUN);
		else if(_tcsicmp(argv[2], TEXT("examples")) == 0)
			_tprintf(HELP_EXAMPLES);
		break;
	}
	return 0;
}
		
int configure(int argc, _TCHAR* argv[])
{
	if(g_config->configure(argc, argv))
		return 0;
	else
		return 1;
}

int clean()
{
	if(g_config->clean())
		return 0;
	else
		return 1;
}


int _tmain(int argc, _TCHAR* argv[])
{
	if(argc < 2 || (argc >=2 && _tcsicmp(argv[1], TEXT("help")) == 0))
	{
		return help(argc, argv);
	}
	else if(argc >= 2)
	{
		g_config = new Win32ClusterdConfig;
		g_config->get(SERVICENAME, &g_serviceName);

		if(_tcsicmp(argv[1], TEXT("run")) == 0)
			return run_as_app();
		else if(_tcsicmp(argv[1], TEXT("install")) == 0)
			return install_service();
		else if(_tcsicmp(argv[1], TEXT("uninstall")) == 0)
			return uninstall_service();
		else if(_tcsicmp(argv[1], TEXT("start")) == 0)
			return start_service();
		else if(_tcsicmp(argv[1], TEXT("stop")) == 0)
			return stop_service();
		else if(_tcsicmp(argv[1], TEXT("service")) == 0)
			return run_as_service();
		else if(_tcsicmp(argv[1], TEXT("configure")) == 0)
			return configure(argc, argv);
		else if(_tcsicmp(argv[1], TEXT("clean")) == 0)
			return clean();
	}
	return 0;
}

void report_service_status(DWORD status, int return_value = 0)
{
	ServiceStatus.dwCurrentState = status;
	ServiceStatus.dwWin32ExitCode = return_value;
	SetServiceStatus(hStatus, &ServiceStatus);
}

void ServiceMain(int argc, char** argv)
{
#ifdef SERVICE_WAIT_FOR_DEBUGGER
	bool wait = true;
	while(wait)
		Sleep(1);
#endif
	DEBUG_ONLY(OutputDebugStringA("ServiceMain()"));
	int error;

	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;

	hStatus = RegisterServiceCtrlHandler(g_serviceName, (LPHANDLER_FUNCTION)ControlHandler);
	if(hStatus == (SERVICE_STATUS_HANDLE)0)
	{
		// Registering control handler failed
		DEBUG_ONLY(OutputDebugStringA("Failed to register control handler\n"));
		return;
	}

	// Initialize Service
	error = InitService();
	if(error)
	{
		DEBUG_ONLY(OutputDebugStringA("Failed to initialize service.\n"));
		report_service_status(SERVICE_STOPPED, -1);
		return;
	}
	
	// Report running status to SCM
	report_service_status(SERVICE_RUNNING);
	
	// the worker loop
	run_loop();
	return; 
}

int InitService()
{
	DEBUG_ONLY(OutputDebugStringA("InitService()\n"));
	return 0;
}

void ControlHandler(DWORD request)
{
	DEBUG_ONLY(OutputDebugStringA("ControlHandler()\n"));
	switch(request)
	{
	case SERVICE_CONTROL_STOP:
		DEBUG_ONLY(OutputDebugStringA("SERVICE_CONTROL_STOP\n"));
	case SERVICE_CONTROL_SHUTDOWN:
		DEBUG_ONLY(OutputDebugStringA("SERVICE_CONTROL_SHUTDOWN\n"));
		report_service_status(SERVICE_STOPPED);
		cluster->kill_all();
		break;
	default:
		DEBUG_ONLY(OutputDebugStringA("Other control request\n"));
		SetServiceStatus(hStatus, &ServiceStatus);
	}
}