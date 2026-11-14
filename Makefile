PREFIX=/usr/local
SUDO=sudo # Leave blank if already superuser
CC = gcc
CFLAGS=-g
LDFLAGS=-lm

all:
	$(CC) $(CFLAGS) main.c -o cplus $(LDFLAGS)

main-clang:
	clang $(CFLAGS) main.c -o cplus $(LDFLAGS)

runWindows: 
	./cplus.exe main.cp

install:
	$(SUDO) mv ./cplus $(PREFIX)/bin

clean:
	rm -fv cplus
