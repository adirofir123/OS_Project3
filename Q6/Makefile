CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
TARGET = server
SRCS = server.c reactor.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean valgrind

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

valgrind: $(TARGET)
	valgrind --leak-check=full --track-origins=yes ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
