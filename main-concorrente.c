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

void* aplicarRegras2(void* ptr);

typedef struct tipoPack {
  int xIni;
  int yIni;
  int z;
} TipoPack;

void aplicarRegras(int nDiv) {
  int qThreads = nDiv*nDiv;
  pthread_t thread[qThreads];
  int  iret[qThreads];

  TipoPack pack[qThreads];

  int resto = nDiv%LINHAS;

  int count = 0;
  for (int i = 0; i < nDiv; i++){
    for (int j = 0; j < nDiv; j++){
      pack[count].z = LINHAS/nDiv;
      pack[count].xIni = i*pack[count].z - 1;
      pack[count].yIni = j*pack[count].z - 1;
      if(resto > 0){
        pack[count].xIni += 1;
        pack[count].yIni += 1;
        resto -= 1;
      }
      
      // printf("%d\n%d\n%d\n\n", pack[count].xIni, pack[count].yIni, pack[count].z);
      iret[count] = pthread_create(&(thread[count]), NULL, aplicarRegras2, (void*) &pack[count]);

      count += 1;
    }
  }

  for (int i = 0; i < nDiv; i++)
    pthread_join(thread[i], NULL);

}

/**
 * @brief Aplica as regras do Jogo da Vida de Conway para calcular a próxima geração.
 * As regras são:
 * 1. Uma célula viva com menos de dois vizinhos vivos morre (subpopulação).
 * 2. Uma célula viva com dois ou três vizinhos vivos sobrevive.
 * 3. Uma célula viva com mais de três vizinhos vivos morre (superpopulação).
 * 4. Uma célula morta com exatamente três vizinhos vivos torna-se viva (reprodução).
 */
void* aplicarRegras2(void* ptr) {
  TipoPack* pack = (TipoPack*) ptr;

  int x,y;
  for (int i = 0; i < pack->z; i++) {
    x = pack->xIni + i;
    for (int j = 0; j < pack->z; j++) {
      y = pack->yIni + j;
      int vizinhosVivos = contarVizinhosVivos(x, y);

      // Regra 1 e 3: Morte por subpopulação ou superpopulação
      if (grid[x][y] == 1 && (vizinhosVivos < 2 || vizinhosVivos > 3)) {
        newGrid[x][y] = 0;
      }
      // Regra 4: Nascimento por reprodução
      else if (grid[x][y] == 0 && vizinhosVivos == 3) {
        newGrid[x][y] = 1;
      }
      // Regra 2: Sobrevivência ou permanência como morta
      else {
        newGrid[x][y] = grid[x][y];
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
    aplicarRegras(1);

    // Atualiza a matriz principal
    atualizarGrid();

    // Opcional: Imprime uma pequena parte da matriz para visualização
    system("clear");
    imprimirGridParcial(40);
  }

  printf("Simulação concluída.\n");

  return 0;
}