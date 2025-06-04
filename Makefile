CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = build/main
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -pthread

clean:
	rm -rf build

.PHONY: all clean
