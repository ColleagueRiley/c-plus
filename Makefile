all: main

CC = gcc

SRCS = main.c

main: $(SRCS)
	$(CC) -g -lm $(SRCS) -o cplus

main-clang: $(SRCS)
	clang -g -lm $(SRCS) -o cplus

main-debug:
	gcc -g -lm main.c -DMEMWATCH -DMW_STDIO memwatch/memwatch.c -o cplus

runWindows: 
	./cplus.exe main.cp

all:
	@make main
	./cplus main.cp -no-compile

install:
	sudo mv ./cplus /usr/bin