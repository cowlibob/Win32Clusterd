#include "stdafx.h"
#include "ServiceControl.h"


ServiceControl::ServiceControl(const TCHAR* serviceName)
	:m_serviceName(serviceName)
{
}


ServiceControl::~ServiceControl(void)
{
	Close();
}

bool ServiceControl::Start()
{
	bool success = false;
	if(Open(SERVICE_START))
	{
		if(!StartService(m_schService, 0, NULL))
		{
			printf("Service could not be started: %d\n", GetLastError());
		}
		else
		{
			printf("Service was started.\n");
			success = true;
		}
		Close();
	}

	return success;
}

bool ServiceControl::Stop()
{
	bool success = true;
	if(Open(SERVICE_STOP))
	{
		SERVICE_STATUS status;
		switch(ControlService(m_schService, SERVICE_CONTROL_STOP, &status))
		{
		case NO_ERROR:
			success = OutputServiceState(status.dwCurrentState);
			break;
		case ERROR_SERVICE_NOT_ACTIVE:
			success = OutputServiceState(status.dwCurrentState);
			break;
		case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
			printf("Service could not be stopped:\n\t");
			success = OutputServiceState(status.dwCurrentState);
			break;
		case ERROR_INVALID_SERVICE_CONTROL:
			printf("Service could not be controlled:\n\t");
			OutputServiceState(status.dwCurrentState);
			success = false;
			break;
		}
	}
	return success;
}

bool ServiceControl::OutputServiceState(DWORD state)
{
	bool success = true;
	switch(state)
	{
	case SERVICE_STOP_PENDING:	printf("Service is now stopping.\n"); break;
	case SERVICE_STOPPED:		printf("Service is stopped.\n"); break;
	default:
		success = false;
	}
	return success;
}

bool ServiceControl::Install(const TCHAR *servicePath)
{

	// Get a handle to the SCM database. 
	m_schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 
 
	if (NULL == m_schSCManager) 
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return false;
	}

	// Create the service
	m_schService = CreateService( 
		m_schSCManager,				// SCM database 
		m_serviceName,				// name of service 
		m_serviceName,				// service name to display 
		SERVICE_ALL_ACCESS,			// desired access 
		SERVICE_WIN32_OWN_PROCESS,	// service type 
		SERVICE_AUTO_START,			// start type 
		SERVICE_ERROR_NORMAL,		// error control type 
		servicePath,						// path to service's binary 
		NULL,						// no load ordering group 
		NULL,						// no tag identifier 
		NULL,						// no dependencies 
		NULL,						// LocalSystem account 
		NULL);						// no password 
 
	if (m_schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(m_schSCManager);
		return false;
	}
	else
		printf("Service installed successfully\n"); 

	// Set the description that is visible in the administration tools server list
	SERVICE_DESCRIPTION sd;
	sd.lpDescription = (TCHAR*)TEXT("Manages multiple ruby processes.");
	BOOL status = ChangeServiceConfig2(m_schService, SERVICE_CONFIG_DESCRIPTION, &sd);
	_ASSERT(status != 0);

	// Ensure that the service restarts in the event of any problems
	SERVICE_FAILURE_ACTIONS	sfa;
	static SC_ACTION failureActions[] = {SC_ACTION_NONE};
	sfa.dwResetPeriod = INFINITE;
	sfa.lpRebootMsg = NULL;
	sfa.lpCommand = NULL;
	sfa.cActions = sizeof(failureActions) / sizeof(failureActions[0]);
	sfa.lpsaActions = failureActions;
	status = ChangeServiceConfig2(m_schService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);
	_ASSERT(status != 0);
	
	CloseServiceHandle(m_schService); 
	CloseServiceHandle(m_schSCManager);	
	return true;
}

bool ServiceControl::Uninstall()
{
	bool success = false;
    if(Open(DELETE))
    { 
		// Delete the service.
 
		if (! DeleteService(m_schService) ) 
		{
			printf("DeleteService failed (%d)\n", GetLastError()); 
		}
		else
		{
			printf("Service unregistered successfully\n"); 
			success = true;
		}
	    Close();
    }

	return success;
}

bool ServiceControl::Open(DWORD desired_access)
{
	m_schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if (NULL == m_schSCManager) 
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return NULL;
    }

    // Get a handle to the service.

    m_schService = OpenService( 
        m_schSCManager,			// SCM database 
        m_serviceName,			// name of service 
        desired_access);

    if (m_schService == NULL)
    { 
        printf("OpenService failed (%d)\n", GetLastError()); 
        CloseServiceHandle(m_schSCManager);
		return false;
    }

	return true;
}

bool ServiceControl::Close()
{
	if(m_schService)
	{
		CloseServiceHandle(m_schService); 
		m_schService = NULL;
	}
	if(m_schSCManager)
	{
		CloseServiceHandle(m_schSCManager);
		m_schSCManager = NULL;
	}
	return true;
}