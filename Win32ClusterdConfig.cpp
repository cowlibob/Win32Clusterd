#include "stdafx.h"

#include "Win32ClusterdConfig.h"
#include <tchar.h>

#ifndef MONGREL_CLUSTER_REG_KEY
#	define MONGREL_CLUSTER_REG_KEY TEXT("SOFTWARE\\SecurStar\\DCE_Server\\Win32Clusterd")
#endif


#define KEY_AND_STRING_FMT TEXT("%[^=]=%s")
#define KEY_AND_NUM_FMT TEXT("%[^=]=%d")

Win32ClusterdConfig::Win32ClusterdConfig(void)
:m_reg_key(NULL)
{
	if(ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, MONGREL_CLUSTER_REG_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &m_reg_key, NULL))
	{
		_tprintf(TEXT("Failed to open or create Registry key %s: <LAST_ERROR>\n"), MONGREL_CLUSTER_REG_KEY);
		m_reg_key = NULL;
	}
}


Win32ClusterdConfig::~Win32ClusterdConfig(void)
{
	RegCloseKey(m_reg_key);
	m_reg_key = NULL;
}


bool Win32ClusterdConfig::write_key(HKEY hKey, const TCHAR *input, const TCHAR *key)
{
	if(ERROR_SUCCESS != RegSetValueEx(hKey, key, NULL, REG_SZ, (BYTE*)input, _tcslen(input) * sizeof(TCHAR)))
	{
		m_log.log(TEXT("Failed to write registry value: <LAST_ERROR>"));
		return false;
	}
	return true;
}

bool Win32ClusterdConfig::write_key(HKEY hKey, const int input, const TCHAR *key)
{
	if(ERROR_SUCCESS != RegSetValueEx(hKey, key, NULL, REG_DWORD, (BYTE*)&input, sizeof(input)))
	{
		m_log.log(TEXT("Failed to write registry value: <LAST_ERROR>"));
		return false;
	}
	return true;
}

bool Win32ClusterdConfig::read_key(HKEY hKey, TCHAR** output, const TCHAR* key )
{
	DWORD output_len = 0;
	DWORD type;
	if(ERROR_SUCCESS != RegQueryValueEx(hKey, key, NULL, &type, NULL, &output_len))
	{
		m_log.log(TEXT("Failed to read registry key: <LAST_ERROR>"));
		return false;
	}

	if(type != REG_SZ)
	{
		m_log.log(TEXT("Registry key %s is not correct type (should be REG_SZ)"), key);
		return false;
	}
	
	*output = new TCHAR[output_len + 1];	// extra for a null-terminator safety-net.
	memset(*output, 0, (output_len + 1) * sizeof(TCHAR));
	//delete[] dst;
	if(ERROR_SUCCESS != RegQueryValueEx(hKey, key, NULL, NULL, (LPBYTE) *output, &output_len))
	{
		m_log.log(TEXT("Failed to read registry key: <LAST_ERROR>"));
		delete[] *output;
		return false;
	}
	
	return true;
}

bool Win32ClusterdConfig::read_key(HKEY hKey, unsigned int* output, const TCHAR* key )
{
	DWORD output_len = 0;
	DWORD type;
	LONG result;
	if(ERROR_SUCCESS != (result = RegQueryValueEx(hKey, key, NULL, &type, NULL, &output_len)))
	{
		m_log.log(TEXT("Failed to query path of ruby binary: <LAST_ERROR>"));
		return false;
	}

	if(type != REG_DWORD)
	{
		m_log.log(TEXT("Registry key %s is not correct type (should be REG_DWORD)"), key);
		return false;
	}
	
	if(ERROR_SUCCESS != (result = RegQueryValueEx(hKey, key, NULL, NULL, (LPBYTE) output, &output_len)))
	{
		m_log.log(TEXT("Failed to query path of ruby binary: <LAST_ERROR>"));
		return false;
	}
	
	return true;
}

bool Win32ClusterdConfig::is_key(const TCHAR* key)
{
	return _tcsicmp(key, MONGREL_CLUSTER_REG_KEY) == 0 ||
		_tcsicmp(key, COMMAND) == 0 ||
		_tcsicmp(key, WORKINGDIR) == 0 ||
		_tcsicmp(key, BASEPORT) == 0 ||
		_tcsicmp(key, INSTANCES) == 0;
}

/*
 * Remove all Win32ClusterdConfig settings from the Registry
 */
bool Win32ClusterdConfig::clean()
{
	bool success = true;
	if(ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, MONGREL_CLUSTER_REG_KEY))
	{
		_tprintf(TEXT("Failed to delete registry key COMPUTER\\HKEY_LOCAL_MACHINE\\%s\n") , MONGREL_CLUSTER_REG_KEY);
		success = false;
	}
	_tprintf(TEXT("Settings successfully removed from COMPUTER\\HKEY_LOCAL_MACHINE\\%s\n"), MONGREL_CLUSTER_REG_KEY);
	return success;
}


/*
 * Configure multiple settings from command line parameters such as instances=3
 */
bool Win32ClusterdConfig::configure(int argc, TCHAR* argv[])
{
	for(int i = 0; i < argc; i++)
		_tprintf(TEXT("%s\n"), argv[i]);

	bool success = true;

	const size_t key_length = 40;
	TCHAR key[key_length];
	memset(key, 0, key_length);
	
	const size_t value_length = 255;
	TCHAR str_value[value_length];
	memset(str_value, 0, value_length);
	
	TCHAR* next_token = NULL;
	int num_value = 0;

	for(int arg_index = 2; arg_index < argc; arg_index++)
	{
		// get both string and numeric versions
		if(_tcsncicmp(argv[arg_index], INSTANCES, _tcslen(INSTANCES)) == 0 || _tcsncicmp(argv[arg_index], BASEPORT, _tcslen(BASEPORT)) == 0)
		{
			_stscanf_s(argv[arg_index], KEY_AND_NUM_FMT, key, 40, &num_value);
			set(key, num_value);
		}
		else if(_tcsncicmp(argv[arg_index], WORKINGDIR, _tcslen(WORKINGDIR)) == 0 || _tcsncicmp(argv[arg_index], COMMAND, _tcslen(COMMAND)) == 0)
		{
			_tcscpy_s(key, key_length, _tcstok_s(argv[arg_index], TEXT("="), &next_token));
			_tcscpy_s(str_value, value_length, _tcstok_s(NULL, TEXT("="), &next_token));
			
			//_stscanf_s(argv[arg_index], KEY_AND_STRING_FMT, key, 40, str_value, 255);
			set(key, str_value);
		}
		else
		{
			_tprintf(TEXT("Unknown configuration: %s\n"), argv[arg_index]);
			success = false;
		}
	}
	success = success && check_complete();
	if(success)
		_tprintf(TEXT("* Settings successfully configured in:\nCOMPUTER\\HKEY_LOCAL_MACHINE\\%s\n"), MONGREL_CLUSTER_REG_KEY);

	return success;
}

bool Win32ClusterdConfig::is_valid(const TCHAR* key)
{
	bool valid = true;
	TCHAR* str = NULL;
	unsigned int num = 0;

	// check each value exists and is not 0 or empty
	if(_tcsicmp(key, COMMAND) == 0 || _tcsicmp(key, WORKINGDIR) == 0)
		valid &= get(key, &str) && _tcslen(str) != 0;
	else if(_tcsicmp(key, INSTANCES) == 0 || _tcsicmp(key, BASEPORT) == 0)
		valid &= get(key, &num) && num != 0;

	if(!valid)
	{
		_tprintf(TEXT("\t%s\n"), key);
		valid = false;
	}
	delete[] str;
	return valid;
}

bool Win32ClusterdConfig::check_complete()
{
	_tprintf(TEXT("\n* Checking configuration:\n"));

	bool valid = is_valid(COMMAND);
	valid &= is_valid(WORKINGDIR);
	valid &= is_valid(INSTANCES);
	valid &= is_valid(BASEPORT);

	if(!valid)
	{
		TCHAR *filename = new TCHAR[255];
		GetModuleFileName(NULL, filename, 100);
		_tprintf(TEXT("above values are missing or invalid. Please run %s configure again.\n"), filename);
		delete[] filename;
	}

	return valid;

}

/*
 * Configure a registry setting
 */
bool Win32ClusterdConfig::set(const TCHAR *key, const TCHAR *value)
{
	bool success = false;
	if(is_key(key))
		success = write_key(m_reg_key, value, key);
	return success;
}

bool Win32ClusterdConfig::set(const TCHAR *key, const unsigned int value)
{
	bool success = false;
	if(is_key(key))
		success = write_key(m_reg_key, value, key);

	return success;
}

bool Win32ClusterdConfig::get(const TCHAR *key, TCHAR **value)
{
	bool success = false;
	if(is_key(key))
		success = read_key(m_reg_key, value, key);

	return success;
}

bool Win32ClusterdConfig::get(const TCHAR *key, unsigned int *value)
{
	bool success = false;
	if(is_key(key))
		success = read_key(m_reg_key, value, key);

	return success;
}
