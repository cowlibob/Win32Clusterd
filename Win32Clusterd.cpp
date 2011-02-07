#include "Win32Clusterd.h"
#include "ProcessDetail.h"
#include <time.h>
#include <tchar.h>
#include "Win32ClusterdConfig.h"


#define SLEEP_TIME 10000
#define MIN_LIFETIME_IN_SECONDS 10
#define MIN_RETRIES_BEFORE_EXIT 1
#define PORT_PLACEHOLDER TEXT("%PORT%")
#define PORT_PLACEHOLDER_LEN 6

Win32Clusterd::Win32Clusterd()
{
	m_average_instance_life = 0;
	m_discard_count = 0;
	m_mongrel_command = NULL;
	m_mongrel_command_template = NULL;
	m_mongrel_working_dir = NULL;
	m_base_port = 0;
	m_desired_instances = 0;
	m_config = new Win32ClusterdConfig;
	m_config->get(COMMAND, &m_mongrel_command_template);
	m_config->get(WORKINGDIR, &m_mongrel_working_dir);
	m_config->get(BASEPORT, &m_base_port);
	m_config->get(INSTANCES, &m_desired_instances);

	// Populate unused ports list
	m_unused_ports.resize(m_desired_instances, m_base_port);
	vector<unsigned int>::iterator it;
	if(!m_unused_ports.empty())
	{
		for( it = m_unused_ports.begin() + 1; it != m_unused_ports.end(); it++)
		{
			*it = *(it - 1) + 1;
		}
	}
}

Win32Clusterd::~Win32Clusterd(void)
{
	m_list.clear();
	m_unused_ports.clear();
	delete[] m_mongrel_command;
	delete[] m_mongrel_command_template;
	delete[] m_mongrel_working_dir;
}

bool Win32Clusterd::is_initialized()
{
	bool init = true;
	init &= m_mongrel_command_template != NULL && _tcslen(m_mongrel_command_template) > 0;
	init &= m_mongrel_working_dir != NULL && _tcslen(m_mongrel_working_dir) > 0;
	init &= m_desired_instances != 0 &&	m_base_port != 0;
	return init;
}

/*
 * Update mongrel cluster, monitoring child processes and launching new as required.
 */
void Win32Clusterd::update()
{
	if(count() < m_desired_instances)
		launch();

	/* Don't launch all instances at once, this has huge system performance impact.
	while(count() < m_desired_instances)
	{
		launch();
	}
	*/
	if(count() > 0)
	{
		monitor();
	}
}
/*
 *	Monitor child processes, discarding exited children.
 */
void Win32Clusterd::monitor()
{
	int instance_count = count();
	ProcessDetailList::iterator it = m_list.begin();

	HANDLE* handles = new HANDLE[instance_count];
	for(unsigned int i = 0; it != m_list.end(); i++, it++)
	{
		handles[i] = (*it).pi->hProcess;
	}

	//check for multiple objects and remove mongrel pi if dead
	DWORD dwEvent = WaitForMultipleObjects( 
		instance_count,     // number of objects in array
		handles,			// array of objects
		FALSE,				// wait for any object
		SLEEP_TIME);		// five-second wait
	delete[] handles;

	int wait_index = dwEvent - WAIT_OBJECT_0;
	int abandon_index = dwEvent - WAIT_ABANDONED_0;
	if(dwEvent == WAIT_TIMEOUT)
	{
		// No processes died, continue
		m_log.log(TEXT("Processes Okay."));
	}
	else if(wait_index < instance_count && wait_index >= 0)
	{
		// object satisfied wait
		// One of the processes died
		discard((unsigned int)wait_index);
	}
	else if(abandon_index < instance_count && abandon_index >= 0)
	{
		// Abandoned mutex satisfied wait
		discard((unsigned int)abandon_index);
	}
	else
	{
		// An error ocurred
		m_log.log(TEXT("Error waiting for event on objects: %WE%"));
	}
}

/*
 * Return current number of child processes (including recently exited processes)
 */
unsigned int Win32Clusterd::count()
{
	unsigned int count = m_list.size();
	m_log.log(TEXT("Counted %d mongrels"), count);
	return count;
}


bool Win32Clusterd::build_process_params(ProcessDetail* pd)
{
	// RubyScriptParams should contain a port placeholder, %PORT%
	TCHAR *substart = _tcsstr(m_mongrel_command_template, PORT_PLACEHOLDER);
	if(!substart)
	{
		m_log.log(TEXT("Failed to find port placeholder in Command registry string."));
		return false;
	}
	
	TCHAR *port = new TCHAR[6];
	wsprintf(port, TEXT("%d"), pd->port);
		
	delete[] m_mongrel_command;
	size_t len = _tcslen(m_mongrel_command_template);
	len *= 2;
	//len += _tcslen(port);
	//len -= PORT_PLACEHOLDER_LEN;
	m_mongrel_command = new TCHAR[len];

	_tcsncpy_s(m_mongrel_command, len, m_mongrel_command_template, substart - m_mongrel_command_template);
	size_t len2 = _tcslen(m_mongrel_command);
	_tcscat_s(m_mongrel_command, len, port);
	size_t len3 = _tcslen(m_mongrel_command);
	_tcscat_s(m_mongrel_command, len, m_mongrel_command_template + (substart - m_mongrel_command_template) + PORT_PLACEHOLDER_LEN);
	size_t len4 = _tcslen(m_mongrel_command);

	return true;
}

/*
 * Launch new child process
 */
bool Win32Clusterd::launch()
{
	m_log.log(TEXT("Launching mongrel"));

	STARTUPINFO si;
	ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

	ProcessDetail* pd = new ProcessDetail(m_unused_ports.back());
	m_unused_ports.pop_back();

	build_process_params(pd);

	bool launched = CreateProcess(NULL, m_mongrel_command, NULL, NULL, FALSE, 0, NULL, m_mongrel_working_dir, &si, pd->pi) == TRUE;

	if(launched)
	{
		m_log.log(TEXT("Mongrel launched!"));
		m_list.push_back(*pd);
		return true;
	}
	else
	{
		m_log.log(TEXT("Mongrel failed to launch."));
		m_unused_ports.push_back(pd->port);
		delete pd;
	}
	return false;
}

/*
 * Discard specified child process.
 * Called by monitor, which has found the process have to exited.
 */
void Win32Clusterd::discard(unsigned int index)
{
	m_log.log(TEXT("Discarding mongrel #%d."), index);

	std::list<ProcessDetail>::iterator it = m_list.begin();
	for(unsigned int i = 0; i < index; i++)
		it ++;

	GetExitCodeProcess(it->pi->hProcess, &(it->exit_code));
	if(it->exit_code != STILL_ACTIVE)
	{
		it->exited = true;

		// Reclaim port
		m_unused_ports.push_back(it->port);
	
		// Statistics
		m_discard_count ++;
		__time64_t now;
		_time64(&now);
		__time64_t lifetime = now - it->started;
		m_log.log(TEXT("Mongrel on port %d died after %ds: pid = %s"), it->port, lifetime, it->pi->dwProcessId);
		update_lifetime_stats(lifetime);

		// And breathe out.
		m_list.erase(it);
	}
}

void Win32Clusterd::kill_all()
{
	while(!m_list.empty())
		m_list.pop_back();
}

/*
 * Keep running average lifetime of child processes
 * Useful to know if they are all exiting too quickly,
 * for example a missing migration.
 */
void Win32Clusterd::update_lifetime_stats(__time64_t lifetime)
{
	__time64_t total_lifetime = (m_average_instance_life * (m_discard_count - 1));
	if(m_discard_count > 0)
		m_average_instance_life = (total_lifetime + lifetime) / m_discard_count;
}

/*
 * Recommends the service should exit if the average
 * child process lifetime is too short.
 */
bool Win32Clusterd::should_quit()
{
	if(	(m_average_instance_life > 0) &&
		(m_average_instance_life < MIN_LIFETIME_IN_SECONDS) &&
		(m_discard_count > MIN_RETRIES_BEFORE_EXIT))
	{
		m_log.log(TEXT("Exiting service as mongrels dying too quickly."));
		return true;
	}
	return false;
}
