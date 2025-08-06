CC = gcc
CFLAGS = -O2 -Wall -lm

# Targets
all: CH_server

CH_server: server.c
	$(CC) $(CFLAGS) -o CH_server server.c

# Clean all
clean:
	rm -f CH_server