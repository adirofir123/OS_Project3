CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g

OBJS = server.o proactor.o

server: $(OBJS)
	$(CC) $(CFLAGS) -o server $(OBJS) -pthread

server.o: server.c proactor.h
	$(CC) $(CFLAGS) -c server.c

proactor.o: proactor.c proactor.h
	$(CC) $(CFLAGS) -c proactor.c

clean:
	rm -f *.o server
