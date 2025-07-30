#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>
#include <mpi/mpi.h>

int TAMANHO;
int NUM_GERACOES;
int NUM_PROCESSOS;

int **grid;
int **newGrid;
int **grid_local;
int **newGrid_local;

// Variáveis para controle MPI
int rank, size;
int linhas_por_processo;
int linhas_locais;
int inicio_local, fim_local;

// Declarações de função
void liberarMatrizes();
void liberarMatrizesLocais();
void atualizarGrid();
void atualizarGridLocal();

void limpeza_handler(int sig __attribute__((unused))) {
    printf("\nInterrompido pelo usuário. Limpando memória...\n");
    liberarMatrizes();
    liberarMatrizesLocais();
    MPI_Finalize();
    exit(0);
}

void alocarMatrizes() {
    if (rank == 0) {
        grid = malloc(TAMANHO * sizeof(int*));
        newGrid = malloc(TAMANHO * sizeof(int*));

        int* grid_data = malloc(TAMANHO * TAMANHO * sizeof(int));
        int* newGrid_data = malloc(TAMANHO * TAMANHO * sizeof(int));

        if (!grid || !newGrid || !grid_data || !newGrid_data) {
            perror("malloc para alocação contígua falhou");
            if (grid) free(grid);
            if (newGrid) free(newGrid);
            if (grid_data) free(grid_data);
            if (newGrid_data) free(newGrid_data);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        for (int i = 0; i < TAMANHO; i++) {
            grid[i] = &grid_data[i * TAMANHO];
            newGrid[i] = &newGrid_data[i * TAMANHO];
        }
    }
}

void alocarMatrizesLocais() {
    // Aloca espaço para as linhas locais + bordas (ghost rows)
    int linhas_com_bordas = linhas_locais + 2; // +2 para bordas superior e inferior

    grid_local = malloc(linhas_com_bordas * sizeof(int*));
    newGrid_local = malloc(linhas_com_bordas * sizeof(int*));

    int* grid_local_data = malloc(linhas_com_bordas * TAMANHO * sizeof(int));
    int* newGrid_local_data = malloc(linhas_com_bordas * TAMANHO * sizeof(int));

    if (!grid_local || !newGrid_local || !grid_local_data || !newGrid_local_data) {
        perror("malloc para alocação local falhou");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    for (int i = 0; i < linhas_com_bordas; i++) {
        grid_local[i] = &grid_local_data[i * TAMANHO];
        newGrid_local[i] = &newGrid_local_data[i * TAMANHO];
    }
}

void liberarMatrizes() {
    if (rank == 0 && grid) {
        free(grid[0]);
        free(grid);
        grid = NULL;
        free(newGrid[0]);
        free(newGrid);
        newGrid = NULL;
    }
}

void liberarMatrizesLocais() {
    if (grid_local) {
        free(grid_local[0]);
        free(grid_local);
        grid_local = NULL;
    }
    if (newGrid_local) {
        free(newGrid_local[0]);
        free(newGrid_local);
        newGrid_local = NULL;
    }
}

void inicializarGrid() {
    if (rank == 0) {
        srand(42); // Seed fixo para reprodutibilidade
        for (int i = 0; i < TAMANHO; i++) {
            for (int j = 0; j < TAMANHO; j++) {
                grid[i][j] = rand() % 2;
            }
        }
    }
}

void distribuirGrid() {
    // Calcula distribuição de linhas
    linhas_por_processo = TAMANHO / size;
    int resto = TAMANHO % size;

    // Arrays para armazenar quantas linhas cada processo recebe e onde começam
    int *sendcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));

    int offset = 0;
    for (int i = 0; i < size; i++) {
        sendcounts[i] = (linhas_por_processo + (i < resto ? 1 : 0)) * TAMANHO;
        displs[i] = offset;
        offset += sendcounts[i];
    }

    // Define limites para este processo
    linhas_locais = linhas_por_processo + (rank < resto ? 1 : 0);
    inicio_local = 0;
    for (int i = 0; i < rank; i++) {
        inicio_local += linhas_por_processo + (i < resto ? 1 : 0);
    }
    fim_local = inicio_local + linhas_locais;

    alocarMatrizesLocais();

    // Distribui dados do grid para todos os processos
    int *buffer_local = malloc(linhas_locais * TAMANHO * sizeof(int));

    if (rank == 0) {
        MPI_Scatterv(grid[0], sendcounts, displs, MPI_INT,
                     buffer_local, linhas_locais * TAMANHO, MPI_INT,
                     0, MPI_COMM_WORLD);
    } else {
        MPI_Scatterv(NULL, NULL, NULL, MPI_INT,
                     buffer_local, linhas_locais * TAMANHO, MPI_INT,
                     0, MPI_COMM_WORLD);
    }

    // Copia dados recebidos para grid_local (índice 1 pois índice 0 é para borda)
    for (int i = 0; i < linhas_locais; i++) {
        memcpy(grid_local[i + 1], &buffer_local[i * TAMANHO], TAMANHO * sizeof(int));
    }

    free(buffer_local);
    free(sendcounts);
    free(displs);
}

void trocarBordas() {
    // Troca bordas superior e inferior com processos vizinhos
    MPI_Request requests[4];
    MPI_Status statuses[4];
    int req_count = 0;

    // Enviar borda inferior para processo seguinte e receber borda superior dele
    if (rank < size - 1) {
        MPI_Isend(grid_local[linhas_locais], TAMANHO, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &requests[req_count++]);
        MPI_Irecv(grid_local[linhas_locais + 1], TAMANHO, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &requests[req_count++]);
    }

    // Enviar borda superior para processo anterior e receber borda inferior dele
    if (rank > 0) {
        MPI_Isend(grid_local[1], TAMANHO, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &requests[req_count++]);
        MPI_Irecv(grid_local[0], TAMANHO, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &requests[req_count++]);
    }

    // Aguarda todas as comunicações
    MPI_Waitall(req_count, requests, statuses);

    // Para bordas do grid (wraparound), copia das bordas opostas
    if (rank == 0) {
        // Primeira linha copia da última linha do último processo
        MPI_Sendrecv(grid_local[1], TAMANHO, MPI_INT, size - 1, 2,
                     grid_local[0], TAMANHO, MPI_INT, size - 1, 3,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (rank == size - 1) {
        // Última linha copia da primeira linha do primeiro processo
        MPI_Sendrecv(grid_local[linhas_locais], TAMANHO, MPI_INT, 0, 3,
                     grid_local[linhas_locais + 1], TAMANHO, MPI_INT, 0, 2,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

int contarVizinhosVivosLocal(int x, int y) {
    // x é relativo ao grid_local (já inclui offset da borda)
    int contagem = 0;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0)
                continue;

            int vizinhoX = x + i;
            int vizinhoY = (y + j + TAMANHO) % TAMANHO; // Wraparound horizontal

            contagem += grid_local[vizinhoX][vizinhoY];
        }
    }
    return contagem;
}

void aplicarRegrasLocais() {
    for (int i = 1; i <= linhas_locais; i++) { // i=1 pois i=0 é borda
        for (int j = 0; j < TAMANHO; j++) {
            int vizinhosVivos = contarVizinhosVivosLocal(i, j);

            if (grid_local[i][j] == 1 && (vizinhosVivos < 2 || vizinhosVivos > 3)) {
                newGrid_local[i][j] = 0;
            } else if (grid_local[i][j] == 0 && vizinhosVivos == 3) {
                newGrid_local[i][j] = 1;
            } else {
                newGrid_local[i][j] = grid_local[i][j];
            }
        }
    }
}

void coletarGrid() {
    // Coleta resultados de todos os processos
    int *sendcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));

    int offset = 0;
    int resto = TAMANHO % size;
    for (int i = 0; i < size; i++) {
        int linhas_proc = linhas_por_processo + (i < resto ? 1 : 0);
        sendcounts[i] = linhas_proc * TAMANHO;
        displs[i] = offset;
        offset += sendcounts[i];
    }

    int *buffer_local = malloc(linhas_locais * TAMANHO * sizeof(int));

    // Copia dados locais para buffer (pula a borda, índice 1)
    for (int i = 0; i < linhas_locais; i++) {
        memcpy(&buffer_local[i * TAMANHO], newGrid_local[i + 1], TAMANHO * sizeof(int));
    }

    if (rank == 0) {
        MPI_Gatherv(buffer_local, linhas_locais * TAMANHO, MPI_INT,
                    newGrid[0], sendcounts, displs, MPI_INT,
                    0, MPI_COMM_WORLD);
    } else {
        MPI_Gatherv(buffer_local, linhas_locais * TAMANHO, MPI_INT,
                    NULL, NULL, NULL, MPI_INT,
                    0, MPI_COMM_WORLD);
    }

    free(buffer_local);
    free(sendcounts);
    free(displs);
}

void atualizarGrid() {
    if (rank == 0) {
        int **temp = grid;
        grid = newGrid;
        newGrid = temp;
    }
}

void atualizarGridLocal() {
    int **temp = grid_local;
    grid_local = newGrid_local;
    newGrid_local = temp;
}

void imprimirGridParcial(int tamanho) {
    if (rank == 0) {
        for (int i = 0; i < tamanho; i++) {
            for (int j = 0; j < tamanho; j++) {
                printf(grid[i][j] ? "■ " : ". ");
            }
            printf("\n");
        }
        printf("\n");
    }
}

double medirTempoMPI() {
    double inicio = MPI_Wtime();

    for (int geracao = 0; geracao < NUM_GERACOES; geracao++) {
        // Troca bordas entre processos
        trocarBordas();

        // Aplica regras localmente
        aplicarRegrasLocais();

        // Coleta resultados no processo 0
        coletarGrid();

        // Atualiza grids
        atualizarGridLocal();
        if (rank == 0) {
            atualizarGrid();
        }

        // Redistribui grid atualizado
        if (geracao < NUM_GERACOES - 1) { // Não precisa redistribuir na última iteração
            if (rank == 0) {
                // Copia newGrid para grid para redistribuição
                for (int i = 0; i < TAMANHO; i++) {
                    memcpy(grid[i], newGrid[i], TAMANHO * sizeof(int));
                }
            }

            // Redistribui o grid atualizado
            distribuirGrid();
        }
    }

    double fim = MPI_Wtime();
    return (fim - inicio) * 1000.0; // Converte para milissegundos
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 4) {
        if (rank == 0) {
            printf("Uso: mpirun -np <num_processos> %s <tamanho> <num_geracoes> <modo>\n", argv[0]);
            printf("Exemplo: mpirun -np 4 %s 1024 50 faixas\n", argv[0]);
            printf("Modo pode ser 'faixas' (implementado) ou 'janelas' (não implementado nesta versão)\n");
        }
        MPI_Finalize();
        return 1;
    }

    TAMANHO = atoi(argv[1]);
    NUM_GERACOES = atoi(argv[2]);
    char* modo = argv[3];
    NUM_PROCESSOS = size;

    if (TAMANHO <= 0 || NUM_GERACOES <= 0) {
        if (rank == 0) {
            printf("Parâmetros inválidos.\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (size > TAMANHO) {
        if (rank == 0) {
            printf("Aviso: Número de processos (%d) maior que tamanho (%d). Reduzindo eficiência.\n", size, TAMANHO);
        }
    }

    if (TAMANHO > 32768) {
        if (rank == 0) {
            printf("Aviso: Tamanho muito grande (%d). Pode causar problemas de memória.\n", TAMANHO);
        }
    }

    if (strcmp(modo, "faixas") != 0) {
        if (rank == 0) {
            printf("Modo inválido ou não implementado. Use 'faixas'.\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        signal(SIGINT, limpeza_handler);
        signal(SIGTERM, limpeza_handler);
    }

    alocarMatrizes();
    inicializarGrid();
    distribuirGrid();

    double tempo_total_ms = medirTempoMPI();

    if (rank == 0) {
        printf("Simulação concluída.\n");
        printf("Tempo total: %.2f ms\n", tempo_total_ms);

        FILE* f = fopen("resultados_mpi.csv", "a");
        if (!f) {
            perror("Erro ao abrir resultados_mpi.csv");
        } else {
            fprintf(f, "%s,%d,%d,%d,%.2f\n", modo, size, TAMANHO, NUM_GERACOES, tempo_total_ms);
            fclose(f);
        }
    }

    liberarMatrizes();
    liberarMatrizesLocais();
    MPI_Finalize();

    return 0;
}
