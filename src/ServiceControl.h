#pragma once
class ServiceControl
{
public:
	ServiceControl(const TCHAR* serviceName);
	~ServiceControl(void);

	bool			Start();
	bool			Stop();
	bool			Install(const TCHAR *servicePath);
	bool			Uninstall();

private:
	bool			Open(DWORD desired_access);
	bool			Close();
	bool			OutputServiceState(DWORD state);

	const TCHAR		*m_serviceName;
	SC_HANDLE		m_schService;
	SC_HANDLE		m_schSCManager;
};

