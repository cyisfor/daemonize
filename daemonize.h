#include "mystring.h"
#include <stdbool.h>

struct daemonize_info {
	string name; 				/* The name of the daemon, base for <name>.pid and <name>.log */
	struct locations {
		/* optional directory locations for the pid and log. Creating them will be attempted.
		   defaults are:
		   for root: /var/run and /var/log
		   else: $HOME/run and $HOME/.local/log
		 */
		string pid;
		string log;
	};
	bool nofork;				/* do not fork. still saves pid file, as the current pid
								   if it forks, the pid file has a (negative) session ID to kill
								*/
	bool dolog;					/* open log files, or just leave stdout/stderr alone? */
	const char* exe_path;		/*
								  optional path to the executable, otherwise
								 argv[1] will be tried.
								*/
	void (*on_first_exit)(void*);
	void (*on_second_exit)(void*);
	void* udata;
	int argc;
	char*const argv[];
};

/* 
	The program takes environment variables as parameters and 
	a command line to be executed on argv.
	There are three options.
	1) provide pid=<file> and log=<file> for the pid and log file locations.
	2) provide name=<pleasenoslashes> for the name, and 
		* if you are root
			it will be /var/run/<name>.pid and /var/log/<name>.log
		* if you are not root
			it will be ~/tmp/run/<name>.pid and ~/.local/logs/<name>.log
	3) provide nothing, but the program executed does not have a complete path, so it can be used as a name, as with (2)
		Such as using "svscan" instead of "/usr/sbin/svscan"
	4) error out
	*/
void daemonize(const struct daemonize_info info) {
