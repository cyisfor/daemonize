#!scons

env = Environment()
# env.ParseConfig('pkg-config gtk+-2.0 --cflags --libs')

objects = list()
objects.append(env.Object("main.c"))

main = env.Program("main",objects)

install = env.InstallAs("/usr/local/bin/daemonize",main)
env.Alias("install",install)