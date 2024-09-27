CC=gcc
CFLAGS= -g -std=gnu11 -Wpedantic -Wall -Werror

all: shell

shell: shell.o 
	$(CC) $(CFLAGS) -o shell shell.o

clean:
	rm -rf shell 
	rm -rf shell.o