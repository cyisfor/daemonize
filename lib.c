// open:
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>

// flock
#include <sys/file.h>

// getpwuid etc
#include <sys/types.h>
#include <pwd.h>

// SIGHUP
#include <signal.h>

#include <stdio.h>
#include <stdlib.h> // realpath
#include <limits.h> // PATH_MAX

#include <assert.h>

#include <unistd.h> // execlp

#include <errno.h>
#include <error.h>

#include <string.h> // memcpy strlen

#include <stdarg.h> // va_arg stuff

#include <stdbool.h> // true/false

static char* build_assured_path(int num, ...) {
	va_list args;
	va_start(args,num);
	int i;
	const char** elements = (const char**) malloc(num*sizeof(const char*));
	ssize_t length = 0;
	for(i=0;i<num;++i) {
		const char* element = va_arg(args,const char*);
		elements[i] = element;
		length += strlen(element);
	}
	char* dest = (char*) malloc(length + (num - 1) + 1);
	char* cur = dest;
	ssize_t pos = 0;
	for(i=0;i<num;++i) {
		cur = stpcpy(cur,elements[i]);
		if(i!=num-1) {
			*(cur++) = '/';
			cur[0] = '\0';
			//printf("Making %s\n",dest);
			if(mkdir(dest,0700)) {
				switch(errno) {
				case EEXIST:
					continue;
				default:
					free(dest);
					free(elements);
					va_end(args);
					return NULL;
				};
			}
		} else {
			cur[0] = '\0';
		}
	}

	free(elements);
	va_end(args);
	
	return dest;
}

static bool noenv(const char* name) {
    const char* value = getenv(name);
    if(value == NULL) return true;
    if(*value == '\0') return true;
    return false;
}

static void die(const char* message) {
  error(errno,errno,"Error is %s",message);
  exit(errno);
}

struct daemonize_info {
	string name;
	struct locations {
		string pid;
		string log;
	};
	bool nofork;
	bool dolog;
	const char* exe_path;
	int argc;
	char*const argv[];
};

	/* This is a little confusing. Let me explain.
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
	ensure_ne(NULL, info.name.base);
	ensure_ge(info.argc, 1);

	char derp[PATH_MAX];
	// must do this BEFORE chdir("/")
	bool need_lookup = false;
	char* exe_path = info.exe_path;
	if(exe_path==NULL) {
		exe_path = realpath(info.argv[1], derp);
	
		if(exe_path==NULL) {
			/* try getting a relative path? */
			struct stat whatever;
			if(0==stat(argv[1],&whatever)) {
				derp[0] = '.';
				derp[1] = '/';
				ssize_t len = strlen(argv[1]);
				if(len > PATH_MAX - 2)
					die("Path too long");		  
				memcpy(derp+2,argv[1],len);
				exe_path = realpath(derp, derp);
			} else {
				// must do path lookup
				need_lookup = true;
				exe_path = argv[1];
			}
		} 
	}

	/* now exe_path is absolute, or we're assuming it's on PATH, so we can chdir */

	const char* home = NULL;

#define BUILD_PATH2(one, oneperm, two, twoperm) ({mkdir(one, oneperm); mkdir(one "/" two, twoperm); (one "/" two);})
	
	uid_t uid = getuid();
	bstring filename = {};
	DEFER { strclear(&filename); }
	straddstr(&filename, info.name);
	const size_t savepoint = filename.len;

#define RESOURCE pid
#define RESOURCE_PERM O_RDWR
	/* not O_EXCL because we're locking this. */
#define RESOURCE_CREATE S_IRUSR|S_IWUSR
#define DEFAULT_ROOT_LOC BUILD_PATH2("/var", 0755, "run", 0755)
#define DEFAULT_USER_LOC BUILD_PATH2("tmp", 0755, "run", 0700);
#include "go_to_location.snippet.c"

	if(flock(pidfd,LOCK_EX|LOCK_NB)==-1) {
	    // if the file is locked, there must already be a daemon running, no need to run!
	    if(errno==EWOULDBLOCK) {
			return false;
		}
	    perror("Lock failed");
	}
	/*
	if the file is not locked, obviously the process that locked
	it must be gone! Even if there is a process of the number in
	the PID file it must just have got that number coincidentally.

	Regardless, we can launch the demon.
	*/

	signal(SIGHUP,SIG_IGN);

	chdir("/");

    if(info.dolog) {
#define RESOURCE log
#define RESOURCE_PERM O_WRONLY|O_APPEND|O_SYNC
	/* not O_EXCL because we're locking this. */
#define RESOURCE_CREATE S_IRUSR|S_IWUSR	
#define DEFAULT_ROOT_LOC BUILD_PATH2("/var", 0755, "log", 0755)
#define DEFAULT_USER_LOC BUILD_PATH2(".local", 0700, "logs", 0700)
#include "go_to_location.snippet.c"	
	
        if(fds.log!=1) {
			dup2(fds.log,1);
		}
		if(fds.log!=2) {
			dup2(fds.log,2);
		}
        if(fds.log!=1&&fds.log!=2) {
			close(fds.log);
			fds.log = -1;
		}
    } else {
        // normally dup2 sets FD_CLOEXEC automatically

        int flags = fcntl(1, F_GETFD);
        if(fcntl(1,F_SETFD,flags & ~FD_CLOEXEC) == -1)
            record(ERROR, "stdout cloexec");
        flags = fcntl(2, F_GETFD);
        if(fcntl(2,F_SETFD,flags & ~FD_CLOEXEC) == -1)
            record(ERROR, "stderr cloexec");
    }
    
    // is O_DSYNC better?
    int flags = fcntl(1, F_GETFL);
    fcntl(1,F_SETFL,O_SYNC|flags);
    flags = fcntl(2, F_GETFL);
    fcntl(2,F_SETFL,O_SYNC|flags);
	fflush(stdout);

    if(info.dolog) {
		/* seriously mega-close stdin */
        int nullfd = open("/dev/null",O_RDONLY);
        assert(nullfd>=0);
        if(nullfd!=0) {
            dup2(nullfd,0);
            close(nullfd);
        }
    }

    int sid = 0;
    if(!info.nofork) {
        setpgrp();

        // First fork.
        int pid = fork();
        if(pid<0) {
			record(ERROR, "First fork failed.");
		} else if(pid>0) {
			if(info.on_first_exit) {
				info.on_first_exit(info.udata);
			}
            exit(0); // parent process exits
		}
		
        // only one process now

        sid = setsid();
        // we want a session ID to kill, to destroy any subsequent forks

        assert(sid >= 0);
    } else {
        sid = getpid(); // hax
    }

	ftruncate(fds.pid,0);
	lseek(fds.pid, 0, SEEK_SET);

    if(!info.nofork) {
    	write(pidfd,"-",1); // make the session ID parse as negative for kill
	}
	char buf[128];
	size_t num = itoa(buf, 128, sid);
	write(fds.pid,buf,num);
	//close(pidfd); don't close, or we lose the lock!
	if(!info.nofork) {
        //second fork
        int pid = fork();
        if(pid < 0) {
			record(ERROR, "Second fork failed");
		} else if(pid > 0) {
			if(info.on_second_exit) {
				info.on_second_exit(info.udata);
			}
          exit(0); // parent process exits
        }
    }
	// only one process now
	// second fork process execs the sub-program.
    
    // these buffers persist through the execvp, somehow...
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

	if(need_lookup) {
	  if(0!=execvp(exe_path,info.argv))
		die(exe_path);
	} else {
	  if(0!=execv(exe_path,info.argv))
		die(exe_path);
	}

	record(ERROR, "Exec failed");
	abort();
}
