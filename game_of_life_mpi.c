#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

// Matrizes locais para cada processo
int **local_grid;
int **local_newGrid;

// Dimensões globais do grid
int TAMANHO_GLOBAL;
int NUM_GERACOES;
char MODO_EXECUCAO[10];

// Dimensões da área local de cada processo
int linhas_locais;
int colunas_locais;

// Topologia 2D para modo janelas
int grid_rows, grid_cols;
int my_row, my_col;

// Funções
void alocarMatrizesLocais(int num_linhas, int num_colunas);
void liberarMatrizesLocais();
int contarVizinhosVivos(int x, int y);
void atualizarGridLocal();
void aplicarRegrasLocal();
void trocarHalosFaixas();
void trocarHalosJanelas();
void calcularTopologia2D(int num_procs, int* rows, int* cols);
void distribuirDadosFaixas(int rank, int num_procs, int **grid_global);
void distribuirDadosJanelas(int rank, int num_procs, int **grid_global);

// Calcula a melhor topologia 2D para o número de processos
void calcularTopologia2D(int num_procs, int* rows, int* cols) {
    int best_diff = num_procs;
    *rows = 1;
    *cols = num_procs;
    
    for (int i = 1; i * i <= num_procs; i++) {
        if (num_procs % i == 0) {
            int j = num_procs / i;
            int diff = abs(i - j);
            if (diff < best_diff) {
                best_diff = diff;
                *rows = i;
                *cols = j;
            }
        }
    }
}

// Aloca as matrizes locais para cada processo, incluindo halos
void alocarMatrizesLocais(int num_linhas, int num_colunas) {
    linhas_locais = num_linhas;
    colunas_locais = num_colunas;
    
    // +2 para halos em ambas as dimensões
    int altura_total = linhas_locais + 2;
    int largura_total = colunas_locais + 2;

    local_grid = malloc(altura_total * sizeof(int*));
    local_newGrid = malloc(altura_total * sizeof(int*));

    int* grid_data = malloc(altura_total * largura_total * sizeof(int));
    int* newGrid_data = malloc(altura_total * largura_total * sizeof(int));

    if (!local_grid || !local_newGrid || !grid_data || !newGrid_data) {
        perror("malloc para alocação local contígua falhou");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    for (int i = 0; i < altura_total; i++) {
        local_grid[i] = &grid_data[i * largura_total];
        local_newGrid[i] = &newGrid_data[i * largura_total];
    }
    
    // Inicializar com zeros
    for (int i = 0; i < altura_total; i++) {
        for (int j = 0; j < largura_total; j++) {
            local_grid[i][j] = 0;
            local_newGrid[i][j] = 0;
        }
    }
}

void liberarMatrizesLocais() {
    if (local_grid) {
        free(local_grid[0]);
        free(local_grid);
        local_grid = NULL;
    }
    if (local_newGrid) {
        free(local_newGrid[0]);
        free(local_newGrid);
        local_newGrid = NULL;
    }
}

// Conta os vizinhos de uma célula no grid local
int contarVizinhosVivos(int x, int y) {
    if (strcmp(MODO_EXECUCAO, "faixas") == 0) {
        // Modo faixas: otimização para bordas horizontais
        if (y > 0 && y < TAMANHO_GLOBAL - 1) {
            return local_grid[x-1][y-1] + local_grid[x-1][y] + local_grid[x-1][y+1] +
                   local_grid[x]  [y-1] +                     local_grid[x]  [y+1] +
                   local_grid[x+1][y-1] + local_grid[x+1][y] + local_grid[x+1][y+1];
        }
        
        // Para bordas horizontais, usa wrap-around
        int contagem = 0;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                
                int vizinhoX = x + i;
                int vizinhoY = (y + j + TAMANHO_GLOBAL) % TAMANHO_GLOBAL;
                
                contagem += local_grid[vizinhoX][vizinhoY];
            }
        }
        return contagem;
    } else {
        // Modo janelas: acesso direto aos halos
        int contagem = 0;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                contagem += local_grid[x + i][y + j];
            }
        }
        return contagem;
    }
}

// Aplica as regras do jogo na área local do processo
void aplicarRegrasLocal() {
    if (strcmp(MODO_EXECUCAO, "faixas") == 0) {
        // Modo faixas: itera apenas sobre as linhas "reais"
        for (int i = 1; i <= linhas_locais; i++) {
            for (int j = 0; j < TAMANHO_GLOBAL; j++) {
                int vizinhosVivos = contarVizinhosVivos(i, j);

                if (local_grid[i][j] == 1 && (vizinhosVivos < 2 || vizinhosVivos > 3)) {
                    local_newGrid[i][j] = 0;
                } else if (local_grid[i][j] == 0 && vizinhosVivos == 3) {
                    local_newGrid[i][j] = 1;
                } else {
                    local_newGrid[i][j] = local_grid[i][j];
                }
            }
        }
    } else {
        // Modo janelas: itera sobre a área 2D local
        for (int i = 1; i <= linhas_locais; i++) {
            for (int j = 1; j <= colunas_locais; j++) {
                int vizinhosVivos = contarVizinhosVivos(i, j);

                if (local_grid[i][j] == 1 && (vizinhosVivos < 2 || vizinhosVivos > 3)) {
                    local_newGrid[i][j] = 0;
                } else if (local_grid[i][j] == 0 && vizinhosVivos == 3) {
                    local_newGrid[i][j] = 1;
                } else {
                    local_newGrid[i][j] = local_grid[i][j];
                }
            }
        }
    }
}

// Troca halos para modo faixas (comunicação vertical)
void trocarHalosFaixas() {
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    
    int vizinho_cima = (rank - 1 + num_procs) % num_procs;
    int vizinho_baixo = (rank + 1) % num_procs;
    
    MPI_Request requests[4];
    MPI_Status statuses[4];
    
    // Troca com vizinho de cima
    MPI_Isend(&local_grid[1][0], TAMANHO_GLOBAL, MPI_INT, vizinho_cima, 0, MPI_COMM_WORLD, &requests[0]);
    MPI_Irecv(&local_grid[0][0], TAMANHO_GLOBAL, MPI_INT, vizinho_cima, 0, MPI_COMM_WORLD, &requests[1]);
    
    // Troca com vizinho de baixo
    MPI_Isend(&local_grid[linhas_locais][0], TAMANHO_GLOBAL, MPI_INT, vizinho_baixo, 0, MPI_COMM_WORLD, &requests[2]);
    MPI_Irecv(&local_grid[linhas_locais + 1][0], TAMANHO_GLOBAL, MPI_INT, vizinho_baixo, 0, MPI_COMM_WORLD, &requests[3]);
    
    MPI_Waitall(4, requests, statuses);
}

// Troca halos para modo janelas (comunicação em 4 direções)
void trocarHalosJanelas() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Comunicação simplificada usando MPI_Barrier para sincronização
    int vizinho_cima = ((my_row - 1 + grid_rows) % grid_rows) * grid_cols + my_col;
    int vizinho_baixo = ((my_row + 1) % grid_rows) * grid_cols + my_col;
    int vizinho_esq = my_row * grid_cols + ((my_col - 1 + grid_cols) % grid_cols);
    int vizinho_dir = my_row * grid_cols + ((my_col + 1) % grid_cols);
    
    // Sincronizar todos os processos antes da comunicação
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Usar apenas comunicação não-bloqueante para evitar deadlock
    MPI_Request requests[8];
    MPI_Status statuses[8];
    int req_count = 0;
    
    // Comunicação vertical
    MPI_Isend(&local_grid[1][1], colunas_locais, MPI_INT, vizinho_cima, rank, MPI_COMM_WORLD, &requests[req_count++]);
    MPI_Irecv(&local_grid[0][1], colunas_locais, MPI_INT, vizinho_cima, vizinho_cima, MPI_COMM_WORLD, &requests[req_count++]);
    
    MPI_Isend(&local_grid[linhas_locais][1], colunas_locais, MPI_INT, vizinho_baixo, rank + 100, MPI_COMM_WORLD, &requests[req_count++]);
    MPI_Irecv(&local_grid[linhas_locais + 1][1], colunas_locais, MPI_INT, vizinho_baixo, vizinho_baixo + 100, MPI_COMM_WORLD, &requests[req_count++]);
    
    // Preparar e enviar dados horizontais
    int* buffer_esq = malloc(linhas_locais * sizeof(int));
    int* buffer_dir = malloc(linhas_locais * sizeof(int));
    int* recv_esq = malloc(linhas_locais * sizeof(int));
    int* recv_dir = malloc(linhas_locais * sizeof(int));
    
    for (int i = 0; i < linhas_locais; i++) {
        buffer_esq[i] = local_grid[i + 1][1];
        buffer_dir[i] = local_grid[i + 1][colunas_locais];
    }
    
    MPI_Isend(buffer_esq, linhas_locais, MPI_INT, vizinho_esq, rank + 200, MPI_COMM_WORLD, &requests[req_count++]);
    MPI_Irecv(recv_esq, linhas_locais, MPI_INT, vizinho_esq, vizinho_esq + 200, MPI_COMM_WORLD, &requests[req_count++]);
    
    MPI_Isend(buffer_dir, linhas_locais, MPI_INT, vizinho_dir, rank + 300, MPI_COMM_WORLD, &requests[req_count++]);
    MPI_Irecv(recv_dir, linhas_locais, MPI_INT, vizinho_dir, vizinho_dir + 300, MPI_COMM_WORLD, &requests[req_count++]);
    
    // Aguardar todas as comunicações
    MPI_Waitall(req_count, requests, statuses);
    
    // Copiar dados recebidos horizontalmente
    for (int i = 0; i < linhas_locais; i++) {
        local_grid[i + 1][0] = recv_esq[i];
        local_grid[i + 1][colunas_locais + 1] = recv_dir[i];
    }
    
    // Aproximação para os cantos usando dados disponíveis
    local_grid[0][0] = local_grid[0][1];
    local_grid[0][colunas_locais + 1] = local_grid[0][colunas_locais];
    local_grid[linhas_locais + 1][0] = local_grid[linhas_locais + 1][1];
    local_grid[linhas_locais + 1][colunas_locais + 1] = local_grid[linhas_locais + 1][colunas_locais];
    
    free(buffer_esq);
    free(buffer_dir);
    free(recv_esq);
    free(recv_dir);
}

// Distribui dados para modo faixas
void distribuirDadosFaixas(int rank, int num_procs, int **grid_global) {
    int *sendcounts = NULL, *displs = NULL;
    
    if (rank == 0) {
        sendcounts = malloc(num_procs * sizeof(int));
        displs = malloc(num_procs * sizeof(int));
        
        int linhas_por_proc = TAMANHO_GLOBAL / num_procs;
        int resto = TAMANHO_GLOBAL % num_procs;
        int offset = 0;
        
        for (int i = 0; i < num_procs; i++) {
            int chunk_size = linhas_por_proc + (i < resto ? 1 : 0);
            sendcounts[i] = chunk_size * TAMANHO_GLOBAL;
            displs[i] = offset;
            offset += sendcounts[i];
        }
    }
    
    MPI_Scatterv(rank == 0 ? grid_global[0] : NULL, sendcounts, displs, MPI_INT,
                 &local_grid[1][0], linhas_locais * TAMANHO_GLOBAL, MPI_INT,
                 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        free(sendcounts);
        free(displs);
    }
}

// Distribui dados para modo janelas (simplificado)
void distribuirDadosJanelas(int rank, int num_procs, int **grid_global) {
    // Versão simplificada: processo 0 envia sequencialmente
    if (rank == 0) {
        // Copia dados para si mesmo
        int start_row = 0;
        int start_col = 0;
        for (int i = 0; i < linhas_locais; i++) {
            for (int j = 0; j < colunas_locais; j++) {
                local_grid[i + 1][j + 1] = grid_global[start_row + i][start_col + j];
            }
        }
        
        // Envia para outros processos
        for (int proc = 1; proc < num_procs; proc++) {
            int proc_row = proc / grid_cols;
            int proc_col = proc % grid_cols;
            
            int proc_linhas_por_janela = TAMANHO_GLOBAL / grid_rows;
            int proc_resto_linhas = TAMANHO_GLOBAL % grid_rows;
            int proc_num_linhas = proc_linhas_por_janela + (proc_row < proc_resto_linhas ? 1 : 0);
            
            int proc_colunas_por_janela = TAMANHO_GLOBAL / grid_cols;
            int proc_resto_colunas = TAMANHO_GLOBAL % grid_cols;
            int proc_num_colunas = proc_colunas_por_janela + (proc_col < proc_resto_colunas ? 1 : 0);
            
            int proc_start_row = proc_row * proc_linhas_por_janela + (proc_row < proc_resto_linhas ? proc_row : proc_resto_linhas);
            int proc_start_col = proc_col * proc_colunas_por_janela + (proc_col < proc_resto_colunas ? proc_col : proc_resto_colunas);
            
            int* buffer = malloc(proc_num_linhas * proc_num_colunas * sizeof(int));
            int idx = 0;
            
            for (int i = 0; i < proc_num_linhas; i++) {
                for (int j = 0; j < proc_num_colunas; j++) {
                    buffer[idx++] = grid_global[proc_start_row + i][proc_start_col + j];
                }
            }
            
            MPI_Send(buffer, proc_num_linhas * proc_num_colunas, MPI_INT, proc, 0, MPI_COMM_WORLD);
            free(buffer);
        }
    } else {
        // Recebe dados do processo 0
        int* buffer = malloc(linhas_locais * colunas_locais * sizeof(int));
        MPI_Recv(buffer, linhas_locais * colunas_locais, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        int idx = 0;
        for (int i = 1; i <= linhas_locais; i++) {
            for (int j = 1; j <= colunas_locais; j++) {
                local_grid[i][j] = buffer[idx++];
            }
        }
        
        free(buffer);
    }
}

// Troca os ponteiros das matrizes locais
void atualizarGridLocal() {
    int **temp = local_grid;
    local_grid = local_newGrid;
    local_newGrid = temp;
}

int main(int argc, char* argv[]) {
    int rank, num_procs;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Somente o processo 0 lê e valida os argumentos de entrada
    if (rank == 0) {
        if (argc != 5) {
            printf("Uso: %s <modo: faixas|janelas> <num_procs> <tamanho> <num_geracoes>\n", argv[0]);
            printf("Exemplo: mpirun -np 4 %s faixas 4 1024 50\n", argv[0]);
            printf("Exemplo: mpirun -np 9 %s janelas 9 1024 50\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        strcpy(MODO_EXECUCAO, argv[1]);
        int num_procs_desejado = atoi(argv[2]);
        TAMANHO_GLOBAL = atoi(argv[3]);
        NUM_GERACOES = atoi(argv[4]);
        
        if (strcmp(MODO_EXECUCAO, "faixas") != 0 && strcmp(MODO_EXECUCAO, "janelas") != 0) {
            printf("Modo inválido. Use 'faixas' ou 'janelas'.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        if (num_procs != num_procs_desejado) {
            printf("Erro: O número de processos MPI (-np %d) não corresponde ao argumento (%d).\n", num_procs, num_procs_desejado);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        if (TAMANHO_GLOBAL <= 0 || NUM_GERACOES <= 0 || num_procs_desejado <= 0) {
            printf("Parâmetros inválidos.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // Verificação específica para modo janelas
        if (strcmp(MODO_EXECUCAO, "janelas") == 0) {
            int test_rows, test_cols;
            calcularTopologia2D(num_procs, &test_rows, &test_cols);
            if (test_rows * test_cols != num_procs) {
                printf("Erro: Número de processos (%d) deve formar uma grade retangular.\n", num_procs);
                printf("Sugestões: 1, 4, 6, 8, 9, 12, 16, 20, 24, 25, 36, 49, 64, 81, 100\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
    }
    
    // Transmitir parâmetros para todos os processos
    MPI_Bcast(MODO_EXECUCAO, 10, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(&TAMANHO_GLOBAL, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&NUM_GERACOES, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Configuração específica por modo
    if (strcmp(MODO_EXECUCAO, "faixas") == 0) {
        // Modo faixas: decomposição 1D horizontal
        grid_rows = num_procs;
        grid_cols = 1;
        my_row = rank;
        my_col = 0;
        
        int linhas_por_proc = TAMANHO_GLOBAL / num_procs;
        int resto = TAMANHO_GLOBAL % num_procs;
        int num_linhas_locais = linhas_por_proc + (rank < resto ? 1 : 0);
        
        alocarMatrizesLocais(num_linhas_locais, TAMANHO_GLOBAL);
        
    } else {
        // Modo janelas: decomposição 2D
        calcularTopologia2D(num_procs, &grid_rows, &grid_cols);
        my_row = rank / grid_cols;
        my_col = rank % grid_cols;
        
        int linhas_por_proc = TAMANHO_GLOBAL / grid_rows;
        int resto_linhas = TAMANHO_GLOBAL % grid_rows;
        int num_linhas_locais = linhas_por_proc + (my_row < resto_linhas ? 1 : 0);
        
        int colunas_por_proc = TAMANHO_GLOBAL / grid_cols;
        int resto_colunas = TAMANHO_GLOBAL % grid_cols;
        int num_colunas_locais = colunas_por_proc + (my_col < resto_colunas ? 1 : 0);
        
        alocarMatrizesLocais(num_linhas_locais, num_colunas_locais);
    }

    int **grid_global = NULL;
    
    // Somente o processo 0 aloca, inicializa o grid e gera dados aleatórios
    if (rank == 0) {
        grid_global = malloc(TAMANHO_GLOBAL * sizeof(int*));
        int* grid_data = malloc(TAMANHO_GLOBAL * TAMANHO_GLOBAL * sizeof(int));
        
        if (!grid_global || !grid_data) {
            printf("Erro: Falha na alocação de memória no processo 0\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        
        for (int i = 0; i < TAMANHO_GLOBAL; i++) {
            grid_global[i] = &grid_data[i * TAMANHO_GLOBAL];
        }

        // Somente o processo 0 gera os dados aleatórios
        srand(42); // Seed fixo para reprodutibilidade
        for (int i = 0; i < TAMANHO_GLOBAL; i++) {
            for (int j = 0; j < TAMANHO_GLOBAL; j++) {
                grid_global[i][j] = rand() % 2;
            }
        }
    }
    
    // Distribuição dos dados
    if (strcmp(MODO_EXECUCAO, "faixas") == 0) {
        distribuirDadosFaixas(rank, num_procs, grid_global);
    } else {
        distribuirDadosJanelas(rank, num_procs, grid_global);
    }

    // Somente o processo 0 libera a matriz global após distribuição
    if (rank == 0) {
        free(grid_global[0]);
        free(grid_global);
        grid_global = NULL;
    }
    
    double tempo_inicio;
    if (rank == 0) {
        tempo_inicio = MPI_Wtime();
    }

    // Loop principal da simulação
    for (int g = 0; g < NUM_GERACOES; g++) {
        // 1. Troca de Halos (específica por modo)
        if (strcmp(MODO_EXECUCAO, "faixas") == 0) {
            trocarHalosFaixas();
        } else {
            trocarHalosJanelas();
        }
        
        // 2. Cálculo Local
        aplicarRegrasLocal();

        // 3. Atualização do Grid
        atualizarGridLocal();
    }

    // Somente o processo 0 calcula o tempo final e salva os resultados
    if (rank == 0) {
        double tempo_fim = MPI_Wtime();
        double tempo_total_s = tempo_fim - tempo_inicio;
        
        printf("Simulação concluída.\n");
        printf("Tempo total: %.2f ms\n", tempo_total_s * 1000.0);

        // Somente o processo 0 escreve no arquivo de resultados
        FILE* f = fopen("resultados_mpi.csv", "a");
        if (f) {
            if (strcmp(MODO_EXECUCAO, "faixas") == 0) {
                fprintf(f, "faixas_mpi,%d,%d,%d,%.2f\n", num_procs, TAMANHO_GLOBAL, NUM_GERACOES, tempo_total_s * 1000.0);
            } else {
                fprintf(f, "janelas_mpi,%d,%d,%d,%.2f\n", num_procs, TAMANHO_GLOBAL, NUM_GERACOES, tempo_total_s * 1000.0);
            }
            fclose(f);
        } else {
            perror("Erro ao abrir resultados_mpi.csv");
        }
    }

    liberarMatrizesLocais();
    MPI_Finalize();

    return 0;
}