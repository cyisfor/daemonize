#include "daemonize.h"

#include "record.h"

#include <stdlib.h> // getenv, strtol

static bool noenv(const char* name) {
    const char* value = getenv(name);
    if(value == NULL) return true;
    if(*value == '\0') return true;
	char* end = NULL;
	if(strtol(value, &end, 0) == 0) {
		if(end && *end == '\0') {
			return true;
		}
	}
    return false;
}

const string maybe_env(const char* name) {
	const char* value = getenv(name);
	if(NULL == value) {
		return ((const string){});
	}
	return strlenstr(value);
}

static void die(const char* message) {
  error(errno,errno,"Error is %s",message);
  exit(errno);
}

int main(int argc, char** argv) {
	const char* name = getenv("name");
	const char* pidFile = getenv("pid");
	const char* logFile;
	/* 
	The program takes environment variables as parameters and 
	a command line to be executed on argv.
	you can either provide name=<pleasenoslashes> for the name, or it will try to get the name
	from the executable path.

	you can provide pid=<location> and log=<location> for the pid and log file locations.
	otherwise:
		* if you are root
			it will be /var/run/ and /var/log/
		* if you are not root
			it will be $HOME/run/ and $HOME/.local/logs/
	NOT the files, since they're always named <name>.pid and <name>.log respectively
	*/
	const struct daemonize_info info = {
		name = maybe_env("name"),
		locations = {
			pid = maybe_env("pid"),
			log = maybe_env("log")
		},
		nofork = ! noenv(getenv("nofork")),
		dolog = ! noenv(getenv("dolog"))
		exe_path = getenv("exe"),
		argc = argc-1,
		argv = argv+1
	};
	daemonize(info);
	record(ERROR, "Daemonize failed.");
	return 0;
}
