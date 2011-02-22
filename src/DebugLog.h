#pragma once

#define MAX_LOG_LEN 1024
class DebugLog
{
public:
	DebugLog(void);
	~DebugLog(void);
	void				log(const TCHAR* msg, ...);
protected:
	const TCHAR *		format_log(const TCHAR *format, va_list arglist);
private:
	TCHAR				m_log[MAX_LOG_LEN+1];
};

