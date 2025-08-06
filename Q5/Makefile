# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g
LDFLAGS = -lpthread

# Project structure
SRC_DIR = .
INC_DIR = .
LIB_DIR = lib
BIN_DIR = bin

# Targets
TARGET = $(BIN_DIR)/reactor_demo
LIBRARY = $(LIB_DIR)/libreactor.a

# Source files
LIB_SRCS = reactor.c
DEMO_SRCS = main.c

# Object files
LIB_OBJS = $(addprefix $(LIB_DIR)/, $(LIB_SRCS:.c=.o))
DEMO_OBJS = $(addprefix $(LIB_DIR)/, $(DEMO_SRCS:.c=.o))

# Header files
HEADERS = reactor.h

.PHONY: all clean directories

all: directories $(LIBRARY) $(TARGET)

# Create directories
directories:
	@mkdir -p $(BIN_DIR) $(LIB_DIR)

# Build library
$(LIBRARY): $(LIB_OBJS)
	ar rcs $@ $^
	@echo "Built library $@"

# Build demo
$(TARGET): $(DEMO_OBJS) $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built executable $@"

# Compile library objects
$(LIB_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@
	@echo "Compiled $<"

# Compile demo objects
$(LIB_DIR)/main.o: $(SRC_DIR)/main.c $(HEADERS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@
	@echo "Compiled $<"

clean:
	rm -rf $(BIN_DIR) $(LIB_DIR)
	@echo "Cleaned build directories"