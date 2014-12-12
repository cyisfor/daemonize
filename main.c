// open:
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>

// getpwuid etc
#include <sys/types.h>
#include <pwd.h>

// SIGHUP
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <unistd.h> // execlp

#include <errno.h>

#include <string.h> // memcpy strlen

#include <stdarg.h> // va_arg stuff

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

int main(int argc, char** argv) {
	const char* name = getenv("name");
	const char* pidFile = getenv("pid");
	const char* logFile;
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
	if(pidFile==NULL) {
		if(name == NULL) {
			if(argc>1 && NULL==strchr(argv[1],'/'))
				name = argv[1];
			else
				return 1;
		}
		uid_t uid = getuid();
		ssize_t nlen = strlen(name);
		char* base = malloc(nlen+5);
		memcpy(base,name,nlen);
		base[nlen+4] = '\0';
		if(uid==0) {
			memcpy(base+nlen,".pid",4);
			pidFile = build_assured_path(4,"/","var","run",base);
			memcpy(base+nlen,".log",4);
			logFile = build_assured_path(4,"/","var","log",base);
		} else {
			struct passwd* u = getpwuid(uid);	
			memcpy(base+nlen,".pid",4);
			pidFile = build_assured_path(4,u->pw_dir,"tmp","run",base);
			memcpy(base+nlen,".log",4);
			logFile = build_assured_path(4,u->pw_dir,".local","logs",base);
		}
		if(!logFile) return 3;
		if(!pidFile) return 4;
		free(base);
	} else {
		logFile = getenv("log");
		if(logFile == NULL) 
			return 2;
	}
		
	int pidfd = open(pidFile,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(pidfd>=0) {
	  if(flock(pidfd,LOCK_EX|LOCK_NB)==-1) {
	    // if the file is locked, there must already be a daemon running!
	    if(errno==EWOULDBLOCK)
	      return 0;
	    perror("Lock failed");
	  }
	    // if the file is not locked, obviously the process that locked
	    // it must be gone! Even if there is a process of the number in
	    // the PID file it must just have got that number coincidentally.
	}

	signal(SIGHUP,SIG_IGN);

	chdir("/");

    if(dolog) {
        int logfd = open(logFile,O_CREAT|O_WRONLY|O_APPEND|O_SYNC,S_IRUSR|S_IWUSR);
        fprintf(stderr,"Got log %d\n",logfd);
        assert(logfd>=0);

        if(logfd!=1) dup2(logfd,1);
        if(logfd!=2) dup2(logfd,2);
        if(logfd!=1&&logfd!=2) close(logfd);
    } else {
        // normally dup2 sets FD_CLOEXEC automatically

        int flags = fcntl(1, F_GETFD);
        if(fcntl(1,F_SETFD,flags & ~FD_CLOEXEC) == -1)
            exit(errno);
        flags = fcntl(2, F_GETFD);
        if(fcntl(2,F_SETFD,flags & ~FD_CLOEXEC) == -1)
            exit(errno);
    }
    
    // is O_DSYNC better?
    int flags = fcntl(1, F_GETFL);
    fcntl(1,F_SETFL,O_SYNC|flags);
    int flags = fcntl(2, F_GETFL);
    fcntl(2,F_SETFL,O_SYNC|flags);
	fflush(stdout);

    if(dolog) {
        int nullfd = open("/dev/null",O_RDONLY);
        assert(nullfd>=0);
        if(nullfd!=0) {
            dup2(nullfd,0);
            close(nullfd);
        }
    }

    int sid = 0;
    if(dofork) {
        setpgrp();

        // First fork.
        int pid = fork();
        if(pid<0) 
            exit(1);
        else if(pid>0) 
            exit(0); // parent process exits

        // only one process now

        sid = setsid();
        // we want a session ID to kill to destroy any subsequent forks

        assert(sid >= 0);
    } else {
        sid = getpid(); // hax
    }

	ftruncate(pidfd,0);
	lseek(pidfd,0,SEEK_SET);
    if(dofork)
    	write(pidfd,"-",1); // make the session ID parse as negative for kill
	char buf[128];
	int num = snprintf(buf,128,"%d",sid);
	write(pidfd,buf,num);
	//close(pidfd); don't close, or we lose the lock!

    if(dofork) {
        //second fork
        pid = fork();
        if(pid < 0)
          exit(1);
        else if(pid > 0) {
          exit(0); // parent process exits
        }
    }
	// only one process now
	// second fork process execs the sub-program.
    
    // these buffers proceed through the execvp apparantly...
    setvbuf(stdout, NULL, _IONBD, 0);
    setvbuf(stderr, NULL, _IONBD, 0);

	execvp(argv[1],argv+1);

	return 0;
}
