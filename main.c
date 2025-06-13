#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#define MATRIX_SIZE 30
#define GENERATIONS 100
#define GET_STATE(cell)         ((cell) & 1)
#define SET_NEXT_ALIVE(cell)    ((cell) |= 2)
#define SET_NEXT_DEAD(cell)     ((cell) &= ~2)
#define APPLY_NEXT_STATE(cell)  ((cell) = ((cell) >> 1) & 1)

uint8_t globalMatrix[MATRIX_SIZE][MATRIX_SIZE];

void simulateGeneration(uint8_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
void printMatrix(uint8_t matrix[MATRIX_SIZE][MATRIX_SIZE]);

int main() {
    globalMatrix[1][2] = 1;
    globalMatrix[2][3] = 1;
    globalMatrix[3][1] = 1;
    globalMatrix[3][2] = 1;
    globalMatrix[3][3] = 1;

    for (size_t i = 0; i < GENERATIONS; i++) {
        printf("\033[H\033[2J");
        printf("Geração %zu\n", i + 1);
        printMatrix(globalMatrix);
        simulateGeneration(globalMatrix);
        usleep(100000);
    }

    return 0;
}

void simulateGeneration(uint8_t matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int x = 0; x < MATRIX_SIZE; x++) {
        for (int y = 0; y < MATRIX_SIZE; y++) {
            int count = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < MATRIX_SIZE && ny >= 0 && ny < MATRIX_SIZE) {
                        count += GET_STATE(matrix[nx][ny]);
                    }
                }
            }
            if (GET_STATE(matrix[x][y])) {
                if (count == 2 || count == 3)
                    SET_NEXT_ALIVE(matrix[x][y]);
                else
                    SET_NEXT_DEAD(matrix[x][y]);
            } else {
                if (count == 3)
                    SET_NEXT_ALIVE(matrix[x][y]);
                else
                    SET_NEXT_DEAD(matrix[x][y]);
            }
        }
    }
daksdmoaksdnmaoisdnmio
    for (int x = 0; x < MATRIX_SIZE; x++) {
        for (int y = 0; y < MATRIX_SIZE; y++) {
            matrix[x][y] = APPLY_NEXT_STATE(matrix[x][y]);
        }
    }
}

void printMatrix(uint8_t matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int x = 0; x < MATRIX_SIZE; x++) {
        for (int y = 0; y < MATRIX_SIZE; y++) {
            printf(GET_STATE(matrix[x][y]) ? "■" : " ");
        }
        printf("\n");
    }
}
