Programs try to handle daemonizing on their own, when it’s a notoriously complicated and error prone process. PID file? Better hope you locked it first! Wait, no not that lock, the other lock. You didn’t know about the other lock? Uh oh, your PID is positive, that’s wrong! You don’t know why that’s wrong? Wow, you just failed to daemonize, and people can’t restart your program reliably now. You did at least double fork, right? Oh, you didn’t setsid until after the second fork, tsk.

This is all pretty ridiculous, since it’s easy to do that stuff in a separate process, then execve the program to be daemonized. Not like runit where a process has to sit around monitoring the daemon somehow, and it can’t close its file descriptors, and has to obey all sorts of rules, this program just daemonizes it, and leaves it be. Since it uses a new session ID, you practically have to re-execute daemonize yourself to stop the PID file from working as a reliable way to tell whether a program has exited or not. If you run daemonize as root, it puts the PID file in /var/run too, which if mounted as tempfs will automatically delete all PID files every reboot.

Proper locking to make sure the PID file has a good session ID in it, proper signalling to make sure that repeating daemonize doesn’t spawn two copies of the same daemon. It basically backgrounds a program, handling logging and session tracking transparently in a way that’s very condusive to running that program as a daemon.


```sh
# daemonize program
# ls /var/run/program.pid
/var/run/program.pid
```
```sh
# daemonize program
# daemonize program
# daemonize program
```
doesn't re-execute program

```sh
# name=something daemonize bash -c ...
# ls /var/run/something.pid
/var/run/something.pid
# ls /var/log/something.log
/var/log/something.log
```

```sh
$ name=guy daemonize program
$ ls ~/tmp/run/guy.pid
$ ls ~/.local/logs/guy.log
```

grep ‘getenv’ in main.c to find all the ways that it can be configured.
