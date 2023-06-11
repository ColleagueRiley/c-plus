all: main

CC = gcc
override CFLAGS += -g -Wno-everything -lm

SRCS = $(shell find . -name '.ccls-cache' -type d -prune -o -type f -name 'main.c' -print)
HEADERS = $(shell find . -name '.ccls-cache' -type d -prune -o -type f -name '*.h' -print)

main: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o cplus

main-debug: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -DMEMWATCH -DMW_STDIO memwatch/memwatch.c -o cplus

all:
	@make main
	./cplus test.cp