#include "daemonize.h"

#include <stdlib.h> // getenv

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

int main(int argc, char** argv) {
	const char* name = getenv("name");
	const char* pidFile = getenv("pid");
	const char* logFile;
	/* 
	The program takes environment variables as parameters and 
	a command line to be executed on argv.
	you can either provide name=<pleasenoslashes> for the name, or it will try to get the name
	from the executable path.

	There are three options.
	1) provide pid=<location> and log=<location> for the pid and log file locations.
	otherwise:
		* if you are root
			it will be /var/run/ and /var/log/
		* if you are not root
			it will be $HOME/run/ and $HOME/.local/logs/
	NOT the files, since they're always named <name>.pid and <name>.log respectively
	3) provide nothing, but the program executed does not have a complete path, so it can be used as a name, as with (2)
		Such as using "svscan" instead of "/usr/sbin/svscan"
	4) error out
	*/
	struct daemonize_info info = {};
	if(getenv("pid")) {
		info.locations.pid 
		if(name == NULL) {
			if(argc>1 && NULL==strchr(argv[1],'/'))
				name = argv[1];
			else
				exit(1);
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
			if(!u)
			  exit(7);
			memcpy(base+nlen,".pid",4);
			pidFile = build_assured_path(4,u->pw_dir,"tmp","run",base);
			memcpy(base+nlen,".log",4);
			logFile = build_assured_path(4,u->pw_dir,".local","logs",base);
		}
		if(!logFile) exit(2);
		if(!pidFile) exit(3);
		free(base);
	} else {
		logFile = getenv("log");
		if(logFile == NULL) 
			exit(4);
	}

    bool dofork = noenv("nofork");
    bool dolog;
    if(!dofork) {
        dolog = ! noenv("dolog");
    } else {
        dolog = true;
    }

    //    printf("dofork %d dolog %d\n",dofork,dolog);
		
	int pidfd = open(pidFile,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(pidfd>=0) {
	  if(flock(pidfd,LOCK_EX|LOCK_NB)==-1) {
	    // if the file is locked, there must already be a daemon running!
	    if(errno==EWOULDBLOCK)
	      exit(0);
	    perror("Lock failed");
	  }
	    // if the file is not locked, obviously the process that locked
	    // it must be gone! Even if there is a process of the number in
	    // the PID file it must just have got that number coincidentally.
	}

	signal(SIGHUP,SIG_IGN);

	char derp[PATH_MAX];
	// must do this BEFORE chdir("/")
	char* exe_path = realpath(argv[1],derp);
	bool need_lookup = false;
	if(exe_path==NULL) {
	  struct stat whatever;
	  if(0==stat(argv[1],&whatever)) {
		derp[0] = '.';
		derp[1] = '/';
		ssize_t len = strlen(argv[1]);
		if(len > PATH_MAX - 2)
		  die("Path too long");		  
		memcpy(derp+2,argv[1],len);
		exe_path = derp;
		argv[1] = exe_path;
	  } else {
		// must do path lookup
		need_lookup = true;
		exe_path = argv[1];
	  }
	} else {
	  argv[1] = exe_path;
	}
	
	chdir("/");

    if(dolog) {
        int logfd = open(logFile,O_CREAT|O_WRONLY|O_APPEND|O_SYNC,S_IRUSR|S_IWUSR);
        assert(logfd>=0);

        if(logfd!=1) dup2(logfd,1);
        if(logfd!=2) dup2(logfd,2);
        if(logfd!=1&&logfd!=2) close(logfd);
    } else {
        // normally dup2 sets FD_CLOEXEC automatically

        int flags = fcntl(1, F_GETFD);
        if(fcntl(1,F_SETFD,flags & ~FD_CLOEXEC) == -1)
            die("stdout cloexec");
        flags = fcntl(2, F_GETFD);
        if(fcntl(2,F_SETFD,flags & ~FD_CLOEXEC) == -1)
            die("stderr cloexec");
    }
    
    // is O_DSYNC better?
    int flags = fcntl(1, F_GETFL);
    fcntl(1,F_SETFL,O_SYNC|flags);
    flags = fcntl(2, F_GETFL);
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
            exit(5);
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
        int pid = fork();
        if(pid < 0)
          exit(6);
        else if(pid > 0) {
          exit(0); // parent process exits
        }
    }
	// only one process now
	// second fork process execs the sub-program.
    
    // these buffers proceed through the execvp apparantly...
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

	if(need_lookup) {
	  if(0!=execvp(exe_path,argv+1))
		die(exe_path);
	} else {
	  if(0!=execv(exe_path,argv+1))
		die(exe_path);
	}

	return 0;
}
