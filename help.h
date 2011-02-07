// Note that a runtime error ocurrs if we dont use double %% in the string:
//		runtime error R6002
//			- floating point support not loaded
// E.G. %PORT% becomes %%PORT%%.

#define HELP_GENERAL TEXT("\nWin32Clusterd.exe [command [options]]\n\nLaunch and maintain a specified number of process instances,\nwhere each concurrent instance is assigned a specific port number.\n\nTo see avaliable commands, type:\n\tWin32Clusterd.exe help commands\n")
#define HELP_COMMANDS TEXT("Available commands:\n\
Configuration:\n\
\tconfigure\n\
\tclean\n\
Service:\n\
\tinstall\n\
\tuninstall\n\
\tstart\n\
\tstop\n\
\trun\n\
\texamples\n\
Type Win32Clusterd help [command] for more information.\
")

#define HELP_CONFIGURE TEXT("Win32Clusterd.exe configure options\n\
\tWhere options are:\n\
\t\tinstances=n\t\t\t- Sets Win32Clusterd to maintain n instances, where n > 0\n\
\t\tbaseport=n\t\t\t- Configures Win32Clusterd to assign (and resuse) consecutive port numbers starting at n\n\
\t\tworkingdir=path\t\t\t- Configures Win32Clusterd child processes to use path as working directory\n\
\t\tcommand=command_line\t\t- Specifies command line for processes to run, eg a path to ruby followed by a script and script options.\n\
\t\t\t\t\t\t  Note that %%PORT%% will be replaced at runtime with the assigned port number.\n")

#define HELP_CLEAN TEXT("Win32Clusterd.exe clean\n\t- removes cluster settings from registry.\n")
#define HELP_INSTALL TEXT("Win32Clusterd.exe install\n\t- installs Win32Clusterd as a windows service.\n")
#define HELP_UNINSTALL TEXT("Win32Clusterd.exe uninstall\n\t- uninstalls Win32Clusterd windows service, if present.\n")
#define HELP_START TEXT("Win32Clusterd.exe start\n\t- starts installed windows service.\n")
#define HELP_STOP TEXT("Win32Clusterd.exe stop\n\t- stops installed windows service.\n")
#define HELP_RUN TEXT("Win32Clusterd.exe run\n\t- runs win32_clusterd as a normal process.\n")
#define HELP_EXAMPLES TEXT("Examples:\n\
\tRun a rails app with c:\\ruby\\bin\\ruby with three instances on ports 300, 3001, 3002:\n\
\t\tWin32Clusterd instances=3 baseport=3000 workingdir=c:\\rails_app command=\"c:\\ruby192\\bin\\ruby.exe c:\\rails_app\\script\\server -p %%PORT%% -e production\n")
