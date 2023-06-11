all: main

CC = gcc
override CFLAGS += -g -Wno-everything -lm

SRCS = main.c

main: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o cplus

main-debug: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -DMEMWATCH -DMW_STDIO memwatch/memwatch.c -o cplus

all:
	@make main
	./cplus test.cp