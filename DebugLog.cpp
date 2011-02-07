#include "stdafx.h"
#include "DebugLog.h"


DebugLog::DebugLog(void)
{
}


DebugLog::~DebugLog(void)
{
}

/*
 * Simple logging to debug stream.
 */
void DebugLog::log(const TCHAR* msg, ...)
{
	memset(m_log, 0, MAX_LOG_LEN * sizeof(TCHAR));
	va_list argptr;
	va_start(argptr, msg);
	format_log(msg, argptr);
	va_end(argptr);
	OutputDebugString(m_log);
	OutputDebugString(TEXT("\n"));
}

/*
 * Message formatting for errors and variable arguments.
 */
const TCHAR * DebugLog::format_log(const TCHAR *format, va_list arglist)
{
	size_t len = 0;

	// Replace <LAST_ERROR> with error message
	const TCHAR *substart = _tcsstr(format, TEXT("<LAST_ERROR>"));
	if (substart != NULL)
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError(); 
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dw, 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);

		// Get size of current string and create a new buffer to expand windows message into it
		size_t fl = _tcslen(format) + _tcslen((const TCHAR *)lpMsgBuf) + 32;		// Add 32 for good measure :-)
		TCHAR *temp = new TCHAR[fl];
		_ASSERT(temp != NULL);
		_tcsncpy_s(temp, fl, format, substart - format);		// Copy the part of the formatting string before the '%winerror%'
		temp[temp - format] = 0;
		_tcscat_s(temp, fl, (const TCHAR *)lpMsgBuf);			// Add the windows error message
		_tcscat_s(temp, fl, substart + 4);					// And the remainder of the original format string

		// Free up the window message
		LocalFree(lpMsgBuf);

		const TCHAR *ret = format_log(temp, arglist);
		delete temp;									// Delete our customised format buffer
		return ret;
	}

	// In case we end up turning fatal exceptions off
	if (m_log != NULL)
	{
		time_t t;
		time(&t);
		tm *today = NULL; // = localtime(&t);
		localtime_s(today, &t);
		len = _tcsftime(m_log, MAX_LOG_LEN, TEXT("[%d-%b-%Y %H:%M:%S] "), today);
		_vsntprintf_s(m_log + len, MAX_LOG_LEN - len, _TRUNCATE, format, arglist);
	}
	return m_log;
}