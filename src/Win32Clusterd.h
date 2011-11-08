#pragma once
#include <list>
#include <vector>
#include <time.h>
#include <Windows.h>
#include "Win32ClusterdConfig.h"
#include "DebugLog.h"
using namespace std;


struct ProcessDetail;
class Win32ClusterdConfig;


class Win32Clusterd
{
public:
	Win32Clusterd(Win32ClusterdConfig* config);
	~Win32Clusterd(void);
	bool				is_initialized();
	void				update();
	bool				should_quit();
	void				kill_all();
protected:
	unsigned int		count();
	bool				launch();
	void				monitor();
	void				discard(unsigned int index);
	void				update_lifetime_stats(__time64_t start);
	void				daily_actions();


	bool				build_process_params(ProcessDetail* pd);
private:
	typedef list<ProcessDetail>	ProcessDetailList;
	ProcessDetailList			m_list;
	unsigned int				m_desired_instances;
	unsigned int				m_base_port;
	vector<unsigned int>		m_unused_ports;
	unsigned int				m_discard_count;
	unsigned int				m_live_count;
	__time64_t					m_average_instance_life;
	TCHAR						*m_mongrel_command;
	TCHAR						*m_mongrel_command_template;
	TCHAR						*m_mongrel_working_dir;
	TCHAR						*m_service_name;
	Win32ClusterdConfig			*m_config;
	DebugLog					m_log;
	WORD						m_current_day;
};

