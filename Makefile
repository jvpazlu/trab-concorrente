CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
LDFLAGS = -lm
EXEC = game_of_life
SRC = game_of_life.c

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC) $(LDFLAGS)

run: all
	./run_simulations.sh

clean:
	rm -f $(EXEC) resultados.csv
