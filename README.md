```sh
# daemonize program
# ls /var/run/program.pid
/var/run/program.pid
# daemonize program
doesn't re-execute program
# name=something daemonize bash -c ...
# ls /var/run/something.pid
/var/run/something.pid
# ls /var/log/something.log
/var/log/something.log
```