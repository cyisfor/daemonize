CFLAGS=-ggdb
PREFIX?=/usr/local/bin

ifneq ($(UID),0)
noroot:=@echo
seteflag:=@echo set -e
gohere:=@echo cd $(PWD)
endif

daemonize: main.o
	gcc $(CFLAGS) -o $@ $^

install: $(PREFIX)/daemonize

$(PREFIX)/daemonize: daemonize
	$(gohere)
	$(noroot) strip $<
	$(noroot) install $< $@
