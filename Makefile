CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99
TARGET = build/main
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -pthread

clean:
	rm -rf build

run: all
	./$(TARGET)

.PHONY: all clean run
