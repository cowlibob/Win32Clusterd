#include "stdafx.h"
#include "ApacheLogRotator.h"
#include "DebugLog.h"
#include "ServiceControl.h"

#define LOG_FILE_EXT _T(".log")
#define LOG_FILE_PATTERN _T("%sold\\%s_%s.log")
#define APACHE_SERVICE_NAME _T("SecurStarDCEhttpd")
ApacheLogRotator::ApacheLogRotator(DebugLog* log, SYSTEMTIME* now)
:	m_log(log),
	m_logs_path_match(NULL),
	/*m_logs_name_pattern(NULL),*/
	m_now(now),
	m_datestamp(NULL),
	m_max_log_history(0),
	m_logs_path(NULL),
	m_serviceControl(new ServiceControl(APACHE_SERVICE_NAME)),
	m_initialized(false)
{
}


ApacheLogRotator::~ApacheLogRotator(void)
{
	if(m_logs_path_match)
		delete[] m_logs_path_match;
	if(m_datestamp)
		delete[] m_datestamp;
	if(m_logs_path)
		delete[] m_logs_path;
	if(m_serviceControl)
		delete m_serviceControl;
}

#ifndef APACHE_LOG_ROTATOR_REG_KEY
#	define APACHE_LOG_ROTATOR_REG_KEY "SOFTWARE\\SecurStar\\DCE_Server\\Apache"
#	define APACHE_LOG_ROTATOR_REG_KEY_W TEXT(APACHE_LOG_ROTATOR_REG_KEY)
#endif

bool ApacheLogRotator::initialize()
{
	if(m_initialized)
		return true;
	
	HKEY reg_key;
	LONG error = RegCreateKeyEx(HKEY_LOCAL_MACHINE, APACHE_LOG_ROTATOR_REG_KEY_W, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &reg_key, NULL);
	if( ERROR_SUCCESS != error)
	{
		if(m_log) m_log->log_error(TEXT("Failed to open or create Registry key %s: <LAST_ERROR>\n"), error, APACHE_LOG_ROTATOR_REG_KEY_W);
		reg_key = NULL;
	}

	if(reg_key == NULL)
	{
		_tprintf(TEXT("Please check read/write permissions for the following registry key:\n\tHKLM\\%s\n"), APACHE_LOG_ROTATOR_REG_KEY_W);
		return false;
	}

	// read history length
	DWORD output_len = sizeof(m_max_log_history);
	DWORD type = 0;
	DWORD result = RegQueryValueEx(reg_key, _T("LogHistoryDays"), NULL, &type, (LPBYTE) &m_max_log_history, &output_len);
	if(ERROR_SUCCESS != result)
	{
		m_log->log_error(TEXT("Failed to query apache log history days from registry: <LAST_ERROR>"), result);
		return false;
	}

	// read log path
	result = RegQueryValueEx(reg_key, _T("LogPathMatch"), NULL, NULL, NULL, &output_len);
	m_logs_path_match = new TCHAR[output_len + 1];
	memset(m_logs_path_match, 0, (output_len + 1) * sizeof(TCHAR));

	error = RegQueryValueEx(reg_key, _T("LogPathMatch"), NULL, NULL, (LPBYTE) m_logs_path_match, &output_len);
	if(ERROR_SUCCESS != error)
	{
		m_log->log_error(TEXT("Failed to query log folder from registry: <LAST_ERROR>"), error);
		delete[] m_logs_path_match;
		m_logs_path_match = NULL;
		return false;
	}
	
	TCHAR* end = _tcsrchr(m_logs_path_match, '\\');
	TCHAR* begin = m_logs_path_match;

	size_t path_len = end - begin + 1;
	m_logs_path = new TCHAR[path_len + 1];
	memset(m_logs_path, 0, (path_len + 1) * sizeof(TCHAR));
	m_logs_path = _tcsncpy(m_logs_path, m_logs_path_match, path_len);

	RegCloseKey(reg_key);

	// generate datestamp
	int chars_written = _sctprintf( _T("%02d%02d%02d_%02d_%02d_%02d"),
				m_now->wYear,
				m_now->wMonth,
				m_now->wDay,
				m_now->wHour,
				m_now->wMinute,
				m_now->wSecond);

	m_datestamp = new TCHAR[chars_written + 1];
	memset(m_datestamp, 0, (chars_written + 1) * sizeof(TCHAR));
	chars_written = _stprintf( m_datestamp, _T("%02d%02d%02d_%02d_%02d_%02d"),
				m_now->wYear,
				m_now->wMonth,
				m_now->wDay,
				m_now->wHour,
				m_now->wMinute,
				m_now->wSecond);
	if(chars_written == -1)
	{
		m_log->log(_T("Failed to write timestamp"));
		return false;
	}

	m_initialized = true;
	return true;
}

bool ApacheLogRotator::rotateLogs()
{
	if(!initialize())
		return false;

#ifndef NO_SERVICE
	if(!stopService())
		return false;
#endif
	// TODO Make this safe and check for folder first.
	int dir_len = _sctprintf(_T("%s\\%s"), m_logs_path, m_datestamp) + 1;
	TCHAR *dir = new TCHAR[dir_len];
	memset(dir, 0, dir_len * sizeof(TCHAR));
	_stprintf(dir, _T("%s\\%s"), m_logs_path, m_datestamp);
	bool dir_created = false;
	WIN32_FIND_DATA data;
	HANDLE h = FindFirstFile(m_logs_path_match, &data);
	while(h != INVALID_HANDLE_VALUE)
	{
		if(!dir_created)
		{
			CreateDirectory(dir, NULL);
			dir_created = true;
		}
		rotateLog(dir, data.cFileName);
		BOOL result = FindNextFile(h, &data);
		if(!result)
		{
			DWORD error = GetLastError();
			if(error == ERROR_NO_MORE_FILES)
			{
				FindClose(h);
				h = INVALID_HANDLE_VALUE;
			}
			else
			{
				if(m_log)
					m_log->log_error(_T("Failed to find next log file: <LAST_ERROR>"), error);
			}
		}
	}
	FindClose(h);
	delete[] dir;
#ifndef NO_SERVICE
	if(!startService())
		return false;
#endif
	return true;
}

bool ApacheLogRotator::rotateLog(const TCHAR *folder, const TCHAR *original)
{
	bool success = true;

	size_t original_len = _tcslen(original) + _tcslen(m_logs_path);
	TCHAR *original_full = new TCHAR[original_len + 1];
	memset(original_full, 0, (original_len + 1) * sizeof(TCHAR));
	_tcscpy(original_full, m_logs_path);
	_tcscat(original_full, original);

	size_t out_len =  _sctprintf(_T("%s\\%s"), folder, original);
	TCHAR *outname = new TCHAR[out_len + 1];
	_stprintf(outname, _T("%s\\%s"), folder, original);

	// copy original to out_name
	if(!MoveFileEx(original_full, outname, MOVEFILE_WRITE_THROUGH))
	{
		if(m_log) m_log->log_error(_T("Failed to move file: <LAST_ERROR>"), GetLastError());
		success = false;
	}
	//delete[] basename;
	delete[] outname;
	delete[] original_full;
	return success;
}

bool ApacheLogRotator::startService()
{
	return m_serviceControl->Start();
}

bool ApacheLogRotator::stopService()
{
	return m_serviceControl->Stop();
}

bool ApacheLogRotator::discardLogs()
{
	if(!initialize())
		return false;

	// calculate oldest logs we want to keep
	FILETIME discard_threshold;
	if(FALSE == SystemTimeToFileTime(m_now, &discard_threshold))
	{
		if(m_log) m_log->log_error(_T("Unable to convert current time to a FILETIME: <LAST_ERROR>"), GetLastError());
		return false;
	}

#define _SECOND ((ULONGLONG) 10000000)
#define _MINUTE (60 * _SECOND)
#define _HOUR   (60 * _MINUTE)
#define _DAY    (24 * _HOUR)


	ULARGE_INTEGER temp;
	temp.HighPart = discard_threshold.dwHighDateTime;
	temp.LowPart = discard_threshold.dwLowDateTime;
#ifndef ACCELERATED_LOG_ROTATION
	temp.QuadPart -= m_max_log_history * _DAY;
#else
	temp.QuadPart -= m_max_log_history * _MINUTE;
#endif
	discard_threshold.dwHighDateTime = temp.HighPart;
	discard_threshold.dwLowDateTime = temp.LowPart;

	// iterate through folders, discarding old ones.
	int pattern_len = _sctprintf(_T("%s\????????_??_??_??"), m_logs_path) + 1;
	TCHAR *pattern = new TCHAR[pattern_len];
	memset(pattern, 0, pattern_len * sizeof(TCHAR));
	_stprintf(pattern, _T("%s\????????_??_??_??"), m_logs_path);

	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(pattern, &data);
	if(hFind == INVALID_HANDLE_VALUE)
	{
		delete[] pattern;
		return false;
	}

	do
	{
		if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{

			if(CompareFileTime(&(data.ftCreationTime), &discard_threshold) < 0)
			{
				int full_path_len = _sctprintf(_T("%s\\%s"), m_logs_path, data.cFileName) + 1;
				TCHAR* full_path = new TCHAR[full_path_len];
				memset(full_path, 0, full_path_len * sizeof(TCHAR));
				_stprintf(full_path, _T("%s\\%s"), m_logs_path, data.cFileName);

				discardFolder(full_path);
				delete[] full_path;
			}
		}
	} while(FindNextFile(hFind, &data));
	FindClose(hFind);
	delete[] pattern;
	return true;

}

bool ApacheLogRotator::discardFolder(const TCHAR *folder)
{
	bool success = true;
	int pattern_len = _sctprintf(_T("%s\\*"), folder) + 1;
	TCHAR *pattern = new TCHAR[pattern_len];
	memset(pattern, 0, pattern_len * sizeof(TCHAR));
	_stprintf(pattern, _T("%s\\*"), folder);

	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(pattern, &data);
	if(hFind == INVALID_HANDLE_VALUE)
	{
		delete[] pattern;
		return false;
	}

	do
	{
		int full_path_len = _sctprintf(_T("%s\\%s"), folder, data.cFileName) + 1;
		TCHAR* full_path = new TCHAR[full_path_len];
		memset(full_path, 0, full_path_len * sizeof(TCHAR));
		_stprintf(full_path, _T("%s\\%s"), folder, data.cFileName);

		if(	_tcscmp(data.cFileName, _T(".")) != 0 &&
			_tcscmp(data.cFileName, _T("..")) != 0)
		{
			if(DeleteFile(full_path) == 0)
			{
				if(m_log) m_log->log_error(_T("Failed to delete file: <LAST_ERROR>"), GetLastError());
			}
		}
		delete[] full_path;
	} while(FindNextFile(hFind, &data));
	FindClose(hFind);
	if(RemoveDirectory(folder) == 0)
	{
		if(m_log) m_log->log_error(_T("Failed to remove folder: <LAST_ERROR>"), GetLastError());
	}
	delete[] pattern;
	return success;
}