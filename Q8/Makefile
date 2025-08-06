CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g
LDFLAGS = -pthread

all: server

server: server.c proactor.c
	$(CC) $(CFLAGS) -o server server.c proactor.c $(LDFLAGS)

clean:
	rm -f server
