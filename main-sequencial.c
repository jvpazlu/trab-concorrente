#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

// Define as dimensões da matriz
#define LINHAS 1000
#define COLUNAS 1000

// Matrizes globais para o estado atual e o próximo estado do jogo
int grid[LINHAS][COLUNAS];
int newGrid[LINHAS][COLUNAS];

/**
 * @brief Inicializa a matriz com um padrão aleatório de células vivas e mortas.
 */
void inicializarGrid() {
  srand(time(NULL));
  for (int i = 0; i < LINHAS; i++) {
    for (int j = 0; j < COLUNAS; j++) {
      // Define uma célula como viva (1) ou morta (0) aleatoriamente
      grid[i][j] = rand() % 2;
    }
  }
}

/**
 * @brief Conta o número de vizinhos vivos de uma célula em uma determinada posição (x, y).
 * As células fora dos limites da matriz são consideradas mortas.
 *
 * @param x A coordenada da linha da célula.
 * @param y A coordenada da coluna da célula.
 * @return O número de vizinhos vivos.
 */
int contarVizinhosVivos(int x, int y) {
  int contagem = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      // Ignora a própria célula
      if (i == 0 && j == 0) {
        continue;
      }

      int vizinhoX = x + i;
      int vizinhoY = y + j;

      if(vizinhoX < 0){
        vizinhoX = LINHAS;
      }else if (vizinhoX == LINHAS){
        vizinhoX = 0;
      }
      if(vizinhoY < 0){
        vizinhoY = COLUNAS;
      }else if (vizinhoY == COLUNAS){
        vizinhoY = 0;
      }

      contagem += grid[vizinhoX][vizinhoY];
    }
  }
  return contagem;
}

/**
 * @brief Aplica as regras do Jogo da Vida de Conway para calcular a próxima geração.
 * As regras são:
 * 1. Uma célula viva com menos de dois vizinhos vivos morre (subpopulação).
 * 2. Uma célula viva com dois ou três vizinhos vivos sobrevive.
 * 3. Uma célula viva com mais de três vizinhos vivos morre (superpopulação).
 * 4. Uma célula morta com exatamente três vizinhos vivos torna-se viva (reprodução).
 */
void aplicarRegras() {
  for (int i = 0; i < LINHAS; i++) {
    for (int j = 0; j < COLUNAS; j++) {
      int vizinhosVivos = contarVizinhosVivos(i, j);

      // Regra 1 e 3: Morte por subpopulação ou superpopulação
      if (grid[i][j] == 1 && (vizinhosVivos < 2 || vizinhosVivos > 3)) {
        newGrid[i][j] = 0;
      }
      // Regra 4: Nascimento por reprodução
      else if (grid[i][j] == 0 && vizinhosVivos == 3) {
        newGrid[i][j] = 1;
      }
      // Regra 2: Sobrevivência ou permanência como morta
      else {
        newGrid[i][j] = grid[i][j];
      }
    }
  }
}

/**
 * @brief Atualiza a matriz principal com o estado da nova geração.
 */
void atualizarGrid() {
  for (int i = 0; i < LINHAS; i++) {
    for (int j = 0; j < COLUNAS; j++) {
      grid[i][j] = newGrid[i][j];
    }
  }
}

void imprimirGridParcial(int tamanho) {
  for (int i = 0; i < tamanho; i++) {
    for (int j = 0; j < tamanho; j++) { //thiago momonga
      printf(grid[i][j] ? "■ " : ". ");
    }
    printf("\n");
  }
  printf("\n");
}

int main() {
  printf("Inicializando a matriz %dx%d...\n", LINHAS, COLUNAS);
  inicializarGrid();

  int numGeracoes = 300;

  printf("Iniciando a simulação para %d gerações...\n", numGeracoes);
  usleep(2000);
  system("clear");
  for (int g = 0; g < numGeracoes; g++) {
    printf("Calculando geração %d...\n", g + 1);

    // Aplica as regras para determinar a próxima geração
    aplicarRegras();

    // Atualiza a matriz principal
    atualizarGrid();

    // Opcional: Imprime uma pequena parte da matriz para visualização
    system("clear");
    imprimirGridParcial(40);
  }

  printf("Simulação concluída.\n");

  return 0;
}