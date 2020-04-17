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
	} locations;
	bool nofork;
/* do not fork. It still saves pid file, as the current pid.
   If it forks, the pid file has a (negative) session ID to kill
*/
	bool nolog;
/* open log files, or just leave stdout/stderr alone? */
	const char* exe_path;
/* optional path to the executable, otherwise
   argv[1] will be tried.
*/
	/* callbacks that are called after the first, and second fork.
	   Should mostly do nothing, and just exit, because you haven't started anything up
	   in your program before calling daemonize. That would be weird, and kind of moronic.
	   No, you just called daemonize right at the top of main, no problem. Right?

	   Otherwise, they should do pre-exit cleanup in the processes that are just going to exit.
	*/
	void (*on_first_exit)(void*);
	void (*on_second_exit)(void*);
	void* udata;

	/* the argv get passed to execv/execvp. argc are just to ensure that argv[1] is a thing,
	   which only matters if you do not provide exe_path.

	   Though it's always an error to exec with an argv of length 0, so good luck with that.
	*/
	int argc;
	char*const*const argv;
};

void daemonize(const struct daemonize_info info);
