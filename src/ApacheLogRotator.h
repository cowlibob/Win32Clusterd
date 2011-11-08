#pragma once

class DebugLog;
class ServiceControl;

class ApacheLogRotator
{
public:
	ApacheLogRotator(DebugLog* log, SYSTEMTIME* now);
	~ApacheLogRotator();
	bool rotateLogs();
	bool discardLogs();
private:
	bool initialize();
	bool stopService();
	bool startService();
	bool rotateLog(const TCHAR* folder, const TCHAR* original);
	bool discardFolder(const TCHAR* folder);

	DebugLog			*m_log;
	ServiceControl		*m_serviceControl;
	TCHAR				*m_logs_path_match;
	TCHAR				*m_logs_path;
	//TCHAR				*m_logs_name_pattern;
	TCHAR				*m_datestamp;
	unsigned int		m_max_log_history;
	SYSTEMTIME			*m_now;
	bool				m_initialized;
};

