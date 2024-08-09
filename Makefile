# Makefile for compiling the HTTP server program

CC = gcc
CLIBS =

all: server.o
	$(CC) -o server server.o $(CLIBS)

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -rf *.o
	rm -rf server
