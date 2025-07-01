#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>

int TAMANHO;
int NUM_GERACOES;

int **grid;
int **newGrid;

// Declarações de função
void liberarMatrizes();
void atualizarGrid();

void limpeza_handler(int sig __attribute__((unused))) {
    printf("\nInterrompido pelo usuário. Limpando memória...\n");
    liberarMatrizes();
    exit(0);
}

void alocarMatrizes() {
    // 1. Alocar o array de ponteiros para as linhas (o "índice")
    grid = malloc(TAMANHO * sizeof(int*));
    newGrid = malloc(TAMANHO * sizeof(int*));

    // 2. Alocar o bloco único e gigante que conterá TODOS os dados da matriz
    int* grid_data = malloc(TAMANHO * TAMANHO * sizeof(int));
    int* newGrid_data = malloc(TAMANHO * TAMANHO * sizeof(int));

    // Verificação de erro para todas as alocações
    if (!grid || !newGrid || !grid_data || !newGrid_data) {
        perror("malloc para alocação contígua falhou");
        // Libera o que quer que tenha sido alocado com sucesso antes de sair
        if (grid) free(grid);
        if (newGrid) free(newGrid);
        if (grid_data) free(grid_data);
        if (newGrid_data) free(newGrid_data);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TAMANHO; i++) {
        grid[i] = &grid_data[i * TAMANHO];
        newGrid[i] = &newGrid_data[i * TAMANHO];
    }
}

void liberarMatrizes() {
    if (grid) {
        free(grid[0]);
        free(grid);
        grid = NULL;
    }
    if (newGrid) {
        free(newGrid[0]);
        free(newGrid);
        newGrid = NULL;
    }
}

void inicializarGrid() {
    srand(42); // Seed fixo para reprodutibilidade
    for (int i = 0; i < TAMANHO; i++) {
        for (int j = 0; j < TAMANHO; j++) {
            grid[i][j] = rand() % 2;
        }
    }
}

int contarVizinhosVivos(int x, int y) {
    if (x > 0 && x < TAMANHO - 1 && y > 0 && y < TAMANHO - 1) {
        return grid[x-1][y-1] + grid[x-1][y] + grid[x-1][y+1] +
               grid[x]  [y-1] +               grid[x]  [y+1] +
               grid[x+1][y-1] + grid[x+1][y] + grid[x+1][y+1];
    }

    int contagem = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0)
                continue;
            int vizinhoX = (x + i + TAMANHO) % TAMANHO;
            int vizinhoY = (y + j + TAMANHO) % TAMANHO;
            contagem += grid[vizinhoX][vizinhoY];
        }
    }
    return contagem;
}

typedef struct {
    int linhaIni;
    int linhaFim;
} TipoPackFaixa;

typedef struct {
    TipoPackFaixa* task_data;
    pthread_barrier_t* barrier;
} ThreadArgsFaixa;

typedef struct {
    int linhaIni;
    int linhaFim;
    int colunaIni;
    int colunaFim;
} TipoPackJanela;

typedef struct {
    TipoPackJanela* task_data;
    pthread_barrier_t* barrier;
} ThreadArgsJanela;


void aplicarRegrasFaixa(void* ptr) {
    TipoPackFaixa* pack = (TipoPackFaixa*) ptr;

    for (int i = pack->linhaIni; i < pack->linhaFim; i++) {
        for (int j = 0; j < TAMANHO; j++) {
            int vizinhosVivos = contarVizinhosVivos(i, j);

            if (grid[i][j] == 1 && (vizinhosVivos < 2 || vizinhosVivos > 3)) {
                newGrid[i][j] = 0;
            } else if (grid[i][j] == 0 && vizinhosVivos == 3) {
                newGrid[i][j] = 1;
            } else {
                newGrid[i][j] = grid[i][j];
            }
        }
    }
}

void* poolControlerFaixa(void* ptr){
    ThreadArgsFaixa* task = (ThreadArgsFaixa*) ptr;
    TipoPackFaixa* pack = task->task_data;
    pthread_barrier_t* barrier = task->barrier;

    for (int i = 0; i < NUM_GERACOES; i++){
        aplicarRegrasFaixa((void*) pack);

        int barrier_result = pthread_barrier_wait(barrier);
        
        if(barrier_result == PTHREAD_BARRIER_SERIAL_THREAD){
            atualizarGrid();
        }
        
        pthread_barrier_wait(barrier);
    }
    free(task);
    pthread_exit(NULL);
}

void aplicarRegrasFaixas(int nThreads) {
    // Limitar número máximo de threads para evitar stack overflow
    if (nThreads > 1000) {
        printf("Erro: Número de threads muito alto (%d). Máximo: 1000\n", nThreads);
        return;
    }

    pthread_t threads[nThreads];
    pthread_barrier_t barrier;

    TipoPackFaixa packs[nThreads];

    int linhasPorThread = TAMANHO / nThreads;
    int resto = TAMANHO % nThreads;
    int linhaAtual = 0;

    for (int i = 0; i < nThreads; i++) {
        packs[i].linhaIni = linhaAtual;
        packs[i].linhaFim = linhaAtual + linhasPorThread + (resto-- > 0 ? 1 : 0);
        linhaAtual = packs[i].linhaFim;
    }

    // Cria a barreira para esperar por nThreads
    if(pthread_barrier_init(&barrier, NULL, nThreads) != 0) {
        perror("Falha ao inicializar a barreira");
    }

    printf("criou a barreira\nLançando as threads:\n\n");
    
    // Cria todas as nThreads threads
    for (int i = 0; i < nThreads; i++){
        ThreadArgsFaixa* task = malloc(sizeof(ThreadArgsFaixa));
        task->task_data = &packs[i];
        task->barrier = &barrier;

        if (pthread_create(&threads[i], NULL, poolControlerFaixa, (void*)task) != 0) {
            perror("Erro ao criar thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < nThreads; i++) {
        pthread_join(threads[i], NULL);
    }
}

void aplicarRegrasJanela(void* ptr) {
    TipoPackJanela* pack = (TipoPackJanela*) ptr;

    for (int i = pack->linhaIni; i < pack->linhaFim; i++) {
        for (int j = pack->colunaIni; j < pack->colunaFim; j++) {
            int vizinhosVivos = contarVizinhosVivos(i, j);

            if (grid[i][j] == 1 && (vizinhosVivos < 2 || vizinhosVivos > 3)) {
                newGrid[i][j] = 0;
            } else if (grid[i][j] == 0 && vizinhosVivos == 3) {
                newGrid[i][j] = 1;
            } else {
                newGrid[i][j] = grid[i][j];
            }
        }
    }
}

void* poolControlerJanela(void* ptr){
    ThreadArgsJanela* task = (ThreadArgsJanela*) ptr;
    TipoPackJanela* pack = task->task_data;
    pthread_barrier_t* barrier = task->barrier;

    for (int i = 0; i < NUM_GERACOES; i++){
        aplicarRegrasJanela((void*) pack);

        // printf("chegou na barreira 1\n");
        // 2. Fica esperando na barreira
        int barrier_result = pthread_barrier_wait(barrier);
        
        if(barrier_result == PTHREAD_BARRIER_SERIAL_THREAD){
            atualizarGrid();
        }
        
        // printf("chegou na barreira 2\n");
        pthread_barrier_wait(barrier);
        // printf("passou da barreira\n");
    }
    free(task);
    pthread_exit(NULL);
}

void aplicarRegrasJanelas(int nDiv) {
    int qThreads = nDiv * nDiv;

    // Limitar número máximo de threads
    if (qThreads > 1000) {
        printf("Erro: Número de threads muito alto (%d). Máximo: 1000\n", qThreads);
        return;
    }

    pthread_t threads[qThreads];
    pthread_barrier_t barrier;

    TipoPackJanela packs[qThreads];

    int linhasPorJanela = TAMANHO / nDiv;
    int colunasPorJanela = TAMANHO / nDiv;
    int restoLinhas = TAMANHO % nDiv;
    int restoColunas = TAMANHO % nDiv;

    int count = 0;
    int linhaIni = 0;

    // 1. Cria os dados para cada thread operar
    for (int i = 0; i < nDiv; i++) {
        int linhaFim = linhaIni + linhasPorJanela + (i < restoLinhas ? 1 : 0);
        int colunaIni = 0;
        for (int j = 0; j < nDiv; j++) {
            int colunaFim = colunaIni + colunasPorJanela + (j < restoColunas ? 1 : 0);

            packs[count].linhaIni = linhaIni;
            packs[count].linhaFim = linhaFim;
            packs[count].colunaIni = colunaIni;
            packs[count].colunaFim = colunaFim;

            // printf("\nlinha ini: %d\nlinha fim: %d\ncoluna ini: %d\ncoluna fim: %d\n\n", packs[count].linhaIni, packs[count].linhaFim, packs[count].colunaIni, packs[count].colunaFim);

            colunaIni = colunaFim;
            count++;
        }
        linhaIni = linhaFim;
    }

    // 2. Cria a barreira para esperar por qThreads
    if(pthread_barrier_init(&barrier, NULL, qThreads) != 0) {
        perror("Falha ao inicializar a barreira");
    }

    printf("criou a barreira\nLançando as threads:\n\n");
    
    // 3. Cria todas as qThreads threads do pool
    for (int i = 0; i < qThreads; i++){
        ThreadArgsJanela* task = malloc(sizeof(ThreadArgsJanela));
        task->task_data = &packs[i];
        task->barrier = &barrier;

        if (pthread_create(&threads[i], NULL, poolControlerJanela, (void*)task) != 0) {
            perror("Erro ao criar thread");
            exit(EXIT_FAILURE);
        }
    }

    // 4. Espera que todas as threads teham acabado de executar
    for (int i = 0; i < qThreads; i++) {
        pthread_join(threads[i], NULL);
    }
}

void atualizarGrid() {
    int **temp = grid;
    grid = newGrid;
    newGrid = temp;
}

void imprimirGridParcial(int tamanho) {
    for (int i = 0; i < tamanho; i++) {
        for (int j = 0; j < tamanho; j++) {
            printf(grid[i][j] ? "■ " : ". ");
        }
        printf("\n");
    }
    printf("\n");
}

// Função para medir apenas o trabalho paralelo
double medirTempoParalelo(char* modo, int numThreads) {
    struct timeval inicio, fim;
    gettimeofday(&inicio, NULL);
    
    if (strcmp(modo, "faixas") == 0) {
        aplicarRegrasFaixas(numThreads);
    } else {
        int nDiv = (int) sqrt(numThreads);
        aplicarRegrasJanelas(nDiv);
    }
    
    gettimeofday(&fim, NULL);
    
    long segundos = fim.tv_sec - inicio.tv_sec;
    long microssegundos = fim.tv_usec - inicio.tv_usec;
    if (microssegundos < 0) {
        segundos--;
        microssegundos += 1000000;
    }
    return segundos * 1000.0 + microssegundos / 1000.0;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Uso: %s <modo: faixas|janelas> <num_threads> <tamanho> <num_geracoes>\n", argv[0]);
        printf("Exemplo: %s faixas 4 1024 50\n", argv[0]);
        return 1;
    }

    char* modo = argv[1];
    int numThreads = atoi(argv[2]);
    TAMANHO = atoi(argv[3]);
    NUM_GERACOES = atoi(argv[4]);

    if (TAMANHO <= 0 || NUM_GERACOES <= 0 || numThreads <= 0) {
        printf("Parâmetros inválidos.\n");
        return 1;
    }

    if (numThreads > TAMANHO) {
        printf("Aviso: Número de threads (%d) maior que tamanho (%d). Reduzindo eficiência.\n", numThreads, TAMANHO);
    }

    if (TAMANHO > 32768) {
        printf("Aviso: Tamanho muito grande (%d). Pode causar problemas de memória.\n", TAMANHO);
    }

    if (strcmp(modo, "janelas") == 0) {
        int raiz = (int) sqrt(numThreads);
        if (raiz * raiz != numThreads) {
            printf("Erro: número de threads (%d) não é um quadrado perfeito para modo janelas.\n", numThreads);
            return 1;
        }
    } else if (strcmp(modo, "faixas") != 0) {
        printf("Modo inválido. Use 'faixas' ou 'janelas'.\n");
        return 1;
    }

    signal(SIGINT, limpeza_handler);
    signal(SIGTERM, limpeza_handler);

    alocarMatrizes();
    inicializarGrid();

    struct timeval inicio, fim;
    double tempo_total_paralelo = 0.0;
    
    gettimeofday(&inicio, NULL);

    // for (int g = 0; g < NUM_GERACOES; g++) {
        // Medir apenas o trabalho paralelo por geração
        double tempo_geracao = medirTempoParalelo(modo, numThreads);
        tempo_total_paralelo += tempo_geracao;

    //     atualizarGrid();

    //     // Impressão apenas para debug (não conta no tempo de benchmark)
    //     // if (TAMANHO <= 40)
    //     //     imprimirGridParcial(TAMANHO);
    // }

    gettimeofday(&fim, NULL);

    // Tempo total incluindo overhead sequencial
    long segundos = fim.tv_sec - inicio.tv_sec;
    long microssegundos = fim.tv_usec - inicio.tv_usec;
    if (microssegundos < 0) {
        segundos--;
        microssegundos += 1000000;
    }
    double tempo_total_ms = segundos * 1000.0 + microssegundos / 1000.0;

    printf("Simulação concluída.\n");
    printf("Tempo total: %.2f ms\n", tempo_total_ms);
    printf("Tempo paralelo puro: %.2f ms (%.1f%%)\n", 
           tempo_total_paralelo, (tempo_total_paralelo/tempo_total_ms)*100);

    FILE* f = fopen("resultados.csv", "a");
    if (!f) {
        perror("Erro ao abrir resultados.csv");
    } else {
        // Salvar tempo total (inclui overhead sequencial)
        fprintf(f, "%s,%d,%d,%d,%.2f\n", modo, numThreads, TAMANHO, NUM_GERACOES, tempo_total_ms);
        fclose(f);
    }

    liberarMatrizes();

    return 0;
}
