# Win32Clusterd
A Windows service responsible for launching and maintaining multiple instances of a specified process.

## Quickstart

`Win32Clusterd configure instances=3 baseport=3000 workingdir=c:\\rails_app command="c:\\ruby192\\bin\\ruby.exe c:\\rails_app\\script\\server -p %PORT% -e production"`
`Win32Clusterd install`
`Win32Clusterd start`

Run a production rails app with `c:\\ruby\\bin\\ruby` with three instances on ports 3000, 3001, 3002

## Usage
`Win32Clusterd.exe [command [options]]`

Launch and maintain a specified number of process instances,
where each concurrent instance is assigned a specific port number.

To see avaliable commands, type:
`Win32Clusterd.exe help commands`

## Available commands
- Configuration
  - configure
  - clean
- Service
  - install
  - uninstall
  - start
  - stop
  - run
  - examples
	
Type `Win32Clusterd help [command]` for more information.


### Configure
`Win32Clusterd.exe configure [options]`

Where options are:

- `instances=n` Sets Win32Clusterd to maintain `n` instances, where `n > 0`
- `baseport=n` Configures Win32Clusterd to assign (and resuse) consecutive port numbers starting at `n`
- `workingdir=path` Configures Win32Clusterd child processes to use path as working directory
- `command=command_line` Specifies command line for processes to run, eg a path to ruby followed by a script and script options. Note that `%PORT%` will be replaced at runtime with the assigned port number.

### Clean
`Win32Clusterd.exe clean`

removes cluster settings from registry.	

### Install
`Win32Clusterd.exe install`

installs Win32Clusterd as a windows service.
	
### Uninstall
`Win32Clusterd.exe uninstall`

uninstalls Win32Clusterd windows service, if present.

### Start
`Win32Clusterd.exe start`

starts installed windows service.

### Stop
`Win32Clusterd.exe stop`

stops installed windows service.

### Run

`Win32Clusterd.exe run`

runs win32_clusterd as a normal process.

