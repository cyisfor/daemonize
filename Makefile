#CFLAGS=-g
PREFIX?=/usr/local/bin

ifeq ($(UID),0)
install=install
else
install=@echo install
endif

daemonize: main.o
	gcc $(CFLAGS) -o $@ $^

install: $(PREFIX)/daemonize

$(PREFIX)/daemonize: daemonize
	$(install) $^ $@
