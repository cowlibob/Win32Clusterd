#include "stdafx.h"

#include "Win32ClusterdConfig.h"
#include <tchar.h>

#ifndef MONGREL_CLUSTER_REG_KEY
#	define MONGREL_CLUSTER_REG_KEY "SOFTWARE\\SecurStar\\DCE_Server\\Win32Clusterd"
#	define MONGREL_CLUSTER_REG_KEY_W TEXT(MONGREL_CLUSTER_REG_KEY)
#endif


#define KEY_AND_STRING_FMT TEXT("%[^=]=%s")
#define KEY_AND_NUM_FMT TEXT("%[^=]=%d")
#define MATCH(_x, _y) (_tcsncicmp(_x, _y, _tcslen(_y)) == 0)

Win32ClusterdConfig::Win32ClusterdConfig(void)
:m_reg_key(NULL)
{
	LONG error = RegCreateKeyEx(HKEY_LOCAL_MACHINE, MONGREL_CLUSTER_REG_KEY_W, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &m_reg_key, NULL);
	if( ERROR_SUCCESS != error)
	{
		m_log.log_error(TEXT("Failed to open or create Registry key %s: <LAST_ERROR>\n"), error, MONGREL_CLUSTER_REG_KEY_W);
		m_reg_key = NULL;
	}

	if(m_reg_key == NULL)
	{
		_tprintf(TEXT("Please check read/write permissions for the following registry key:\n\tHKLM\\%s\n"), MONGREL_CLUSTER_REG_KEY_W);
		_exit(error);
	}

}


Win32ClusterdConfig::~Win32ClusterdConfig(void)
{
	RegCloseKey(m_reg_key);
	m_reg_key = NULL;
}


bool Win32ClusterdConfig::write_key(HKEY hKey, const TCHAR *input, const TCHAR *key)
{
	LONG error = RegSetValueEx(hKey, key, NULL, REG_SZ, (BYTE*)input, _tcslen(input) * sizeof(TCHAR));
	if(ERROR_SUCCESS != error)
	{
		m_log.log_error(TEXT("Failed to write registry value: <LAST_ERROR>"), error);
		return false;
	}
	return true;
}

bool Win32ClusterdConfig::write_key(HKEY hKey, const int input, const TCHAR *key)
{
	LONG error = RegSetValueEx(hKey, key, NULL, REG_DWORD, (BYTE*)&input, sizeof(input));
	if(ERROR_SUCCESS !=  error)
	{
		m_log.log_error(TEXT("Failed to write registry value: <LAST_ERROR>"), error);
		return false;
	}
	return true;
}

bool Win32ClusterdConfig::read_key(HKEY hKey, TCHAR** output, const TCHAR* key )
{
	DWORD output_len = 0;
	DWORD type;
	LONG error = RegQueryValueEx(hKey, key, NULL, &type, NULL, &output_len);
	if(ERROR_SUCCESS != error)
	{
		m_log.log_error(TEXT("Failed to read registry key: <LAST_ERROR>"), error);
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
	error = RegQueryValueEx(hKey, key, NULL, NULL, (LPBYTE) *output, &output_len);
	if(ERROR_SUCCESS != error)
	{
		m_log.log_error(TEXT("Failed to read registry key: <LAST_ERROR>"), error);
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
	result = RegQueryValueEx(hKey, key, NULL, &type, NULL, &output_len);
	if(ERROR_SUCCESS != result)
	{
		m_log.log_error(TEXT("Failed to query path of ruby binary: <LAST_ERROR>"), result);
		return false;
	}

	if(type != REG_DWORD)
	{
		m_log.log(TEXT("Registry key %s is not correct type (should be REG_DWORD)"), key);
		return false;
	}
	
	result = RegQueryValueEx(hKey, key, NULL, NULL, (LPBYTE) output, &output_len);
	if(ERROR_SUCCESS != result)
	{
		m_log.log_error(TEXT("Failed to query path of ruby binary: <LAST_ERROR>"), result);
		return false;
	}
	
	return true;
}

bool Win32ClusterdConfig::is_key(const TCHAR* key)
{
	return _tcsicmp(key, MONGREL_CLUSTER_REG_KEY_W) == 0 ||
		_tcsicmp(key, COMMAND) == 0 ||
		_tcsicmp(key, WORKINGDIR) == 0 ||
		_tcsicmp(key, BASEPORT) == 0 ||
		_tcsicmp(key, SERVICENAME) == 0 ||
		_tcsicmp(key, INSTANCES) == 0;
}

/*
 * Remove all Win32ClusterdConfig settings from the Registry
 */
bool Win32ClusterdConfig::clean()
{
	bool success = true;
	if(ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, MONGREL_CLUSTER_REG_KEY_W))
	{
		_tprintf(TEXT("Failed to delete registry key COMPUTER\\HKEY_LOCAL_MACHINE\\%s\n") , MONGREL_CLUSTER_REG_KEY_W);
		success = false;
	}
	_tprintf(TEXT("Settings successfully removed from COMPUTER\\HKEY_LOCAL_MACHINE\\%s\n"), MONGREL_CLUSTER_REG_KEY_W);
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
		TCHAR* arg = argv[arg_index];
		if( MATCH(arg, INSTANCES) || MATCH(arg, BASEPORT) )
		{
			_stscanf_s(arg, KEY_AND_NUM_FMT, key, 40, &num_value);
			success = success && set(key, num_value);
		}
		else if( MATCH(arg, WORKINGDIR) || MATCH(arg, COMMAND) || MATCH(arg, SERVICENAME) )
		{
			_tcscpy_s(key, key_length, _tcstok_s(arg, TEXT("="), &next_token));
			_tcscpy_s(str_value, value_length, _tcstok_s(NULL, TEXT("="), &next_token));
			
			//_stscanf_s(argv[arg_index], KEY_AND_STRING_FMT, key, 40, str_value, 255);
			success = success && set(key, str_value);
		}
		else
		{
			_tprintf(TEXT("Unknown configuration: %s\n"), arg);
			success = false;
		}
	}
	success = success && check_complete();
	if(success)
		_tprintf(TEXT("* Settings successfully configured in:\nCOMPUTER\\HKEY_LOCAL_MACHINE\\%s\n"), MONGREL_CLUSTER_REG_KEY_W);

	return success;
}

bool Win32ClusterdConfig::is_valid(const TCHAR* key)
{
	bool valid = true;
	TCHAR* str = NULL;
	unsigned int num = 0;

	// check each value exists and is not 0 or empty
	if( MATCH(key, COMMAND) || MATCH(key, WORKINGDIR) || MATCH(key, SERVICENAME) )
		valid &= get(key, &str) && _tcslen(str) != 0;
	else if( MATCH(key, INSTANCES) || MATCH(key, BASEPORT))
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
	valid &= is_valid(SERVICENAME);

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
