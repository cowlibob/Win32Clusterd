#pragma once
#include "DebugLog.h"

#ifndef COMMAND
#	define COMMAND TEXT("Command")
#endif
#ifndef WORKINGDIR
#	define WORKINGDIR TEXT("WorkingDir")
#endif
#ifndef BASEPORT
#	define BASEPORT TEXT("BasePort")
#endif
#ifndef INSTANCES
#	define INSTANCES TEXT("Instances")
#endif
#ifndef SERVICENAME
#	define SERVICENAME TEXT("ServiceName")
#endif

class Win32ClusterdConfig
{
public:
	Win32ClusterdConfig(void);
	~Win32ClusterdConfig(void);
	bool				configure(int argc, TCHAR* argv[]);
	bool				clean();
	bool				set(const TCHAR *key, const TCHAR *value);
	bool				set(const TCHAR *key, const unsigned int value);
	bool				get(const TCHAR *key, TCHAR **output);
	bool				get(const TCHAR *key, unsigned int *output);
protected:
	bool				write_key(HKEY hKey, const TCHAR *input, const TCHAR *key);
	bool				write_key(HKEY hKey, const int input, const TCHAR *key);
	bool				read_key(HKEY hKey, TCHAR **output, const TCHAR *key);
	bool				read_key(HKEY hKey, unsigned int *output, const TCHAR *key);
	bool				is_key(const TCHAR* key);
	bool				check_complete();
	bool				is_valid(const TCHAR* key);
private:
	HKEY						m_reg_key;
	DebugLog					m_log;
};

