all: main

CC = gcc

SRCS = main.c

main: $(SRCS)
	$(CC)  -g -lm $(SRCS) -o cplus

main-debug: $(SRCS)
	$(CC)  -g -lm $(SRCS) -DMEMWATCH -DMW_STDIO memwatch/memwatch.c -o cplus

runWindows: 
	./cplus.exe test.cp

all:
	@make main
	./cplus test.cp