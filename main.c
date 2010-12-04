// open:
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>

// SIGHUP
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <unistd.h> // execlp

#include <errno.h>

int main(int argc, char** argv) {
	assert(argc>3);

	const char* pidFile = argv[1];

	int pidfd = open(pidFile,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(pidfd>=0) {
	  if(flock(pidfd,LOCK_EX|LOCK_NB)==-1) {
	    // if the file is locked, there must already be a daemon running!
	    if(errno==EWOULDBLOCK)
	      exit(0);
	    perror("Lock failed");
	  }
	    // if the file is not locked, obviously the process that locked
	    // it must be gone!
	    /*
	    char buf[128];
	    ssize_t size = read(fd,buf,128);
	    close(fd);
	    if(size > 0) {
	      buf[size] = '\0';
	      int pid = atoi(buf);
	      if(pid > 0 && kill(pid,0)==0) {
		printf("Process %d already exists if incorrect, delete %s and try again.\n",pid,pidFile);
		exit(0);
	      }
	      }*/
	}

	signal(SIGHUP,SIG_IGN);

	// First fork.
	int pid = fork();
	if(pid<0) 
	  exit(1);
	else if(pid>0) 
	  exit(0); // parent process exits

	// only one process now

	chdir("/");
	assert(setsid() >= 0);
	const char* log = argv[2];

	int logfd = open(log,O_CREAT|O_WRONLY|O_APPEND|O_SYNC,S_IRUSR|S_IWUSR);
	assert(logfd>=0);

	if(logfd!=1) dup2(logfd,1);
	if(logfd!=2) dup2(logfd,2);
	if(logfd!=1&&logfd!=2) close(logfd);

	fflush(stdout);

	int nullfd = open("/dev/null",O_RDONLY);
	assert(nullfd>=0);
	if(nullfd!=0) {
	  dup2(nullfd,0);
	  close(nullfd);
	}

	//second fork
	pid = fork();
	if(pid < 0)
	  exit(1);
	else if(pid > 0) {
	  // first fork writes the PID of the second fork.
	  ftruncate(pidfd,0);
	  lseek(pidfd,0,SEEK_SET);
	  char buf[128];
	  int num = snprintf(buf,128,"%d",pid);
	  write(pidfd,buf,num);
	  //close(fd); don't close, or we lose the lock!
	  exit(0); // parent process exits
	}

	// only one process now
	// second fork process execs the sub-program.

	execvp(argv[3],argv+3);

	return 0;
}
