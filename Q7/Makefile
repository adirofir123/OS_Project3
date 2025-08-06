CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g
LDFLAGS = -pthread

all: server convex_hull

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

convex_hull: convex_hull.c
	$(CC) $(CFLAGS) -o convex_hull convex_hull.c

clean:
	rm -f server convex_hull
