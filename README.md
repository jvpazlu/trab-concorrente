# Game of Life - Implementações Paralelas

Este repositório contém implementações paralelas do Jogo da Vida de Conway usando **pthread** e **MPI**.

## 📁 Arquivos Principais

### Códigos Fonte
- `game_of_life.c` - Implementação pthread (faixas e janelas)
- `game_of_life_mpi.c` - Implementação MPI (faixas e janelas)

### Scripts de Simulação
- `run_pthread_simulations.sh` - Executa testes pthread completos
- `run_mpi_simulations.sh` - Executa testes MPI completos

### Arquivos de Build
- `Makefile` - Compilação automática

## 🚀 Como Usar

### Compilação
```bash
# Pthread
gcc -o game_of_life game_of_life.c -lm -lpthread

# MPI
mpicc -o game_of_life_mpi game_of_life_mpi.c -lm
```

### Execução Manual

#### Pthread
```bash
# Modo faixas
./game_of_life faixas <num_threads> <tamanho> <num_geracoes>

# Modo janelas (threads deve ser quadrado perfeito)
./game_of_life janelas <num_threads> <tamanho> <num_geracoes>

# Exemplos
./game_of_life faixas 4 1024 50
./game_of_life janelas 9 1024 50
```

#### MPI
```bash
# Modo faixas
mpirun -np <num_procs> ./game_of_life_mpi faixas <num_procs> <tamanho> <num_geracoes>

# Modo janelas (procs deve formar grade retangular)
mpirun -np <num_procs> ./game_of_life_mpi janelas <num_procs> <tamanho> <num_geracoes>

# Exemplos
mpirun -np 4 ./game_of_life_mpi faixas 4 1024 50
mpirun -np 9 ./game_of_life_mpi janelas 9 1024 50
```

### Execução dos Benchmarks Completos

#### Pthread
```bash
chmod +x run_pthread_simulations.sh
./run_pthread_simulations.sh
```
- **Saída**: `resultados_pthread.csv`
- **Testes**: 140 simulações (70 faixas + 70 janelas)

#### MPI
```bash
chmod +x run_mpi_simulations.sh
./run_mpi_simulations.sh
```
- **Saída**: `resultados_mpi.csv`
- **Testes**: 140 simulações (70 faixas + 70 janelas)

## 📊 Parâmetros dos Benchmarks

### Configuração
- **Gerações**: 100
- **Tamanhos**: 256, 512, 1024, 2048, 4096, 8192, 16384
- **Workers**: 1, 4, 9, 16, 25, 36, 49, 64, 81, 100

### Restrições
- **Janelas pthread**: Número de threads deve ser quadrado perfeito
- **Janelas MPI**: Número de processos deve formar grade retangular
- **Timeout**: 300 segundos por simulação

## 📈 Formato dos Resultados

### CSV Pthread (`resultados_pthread.csv`)
```csv
modo,num_threads,tamanho,num_geracoes,tempo_ms
faixas,4,1024,100,245.67
janelas,9,1024,100,189.23
```

### CSV MPI (`resultados_mpi.csv`)
```csv
modo,num_procs,tamanho,num_geracoes,tempo_ms
faixas_mpi,4,1024,100,198.45
janelas_mpi,9,1024,100,156.78
```

## 🔧 Algoritmos Implementados

### Pthread
- **Faixas**: Decomposição 1D horizontal com threads
- **Janelas**: Decomposição 2D em blocos com threads
- **Sincronização**: Barreiras customizadas

### MPI
- **Faixas**: Decomposição 1D horizontal com processos
- **Janelas**: Decomposição 2D em grade com processos
- **Comunicação**: Troca de halos não-bloqueante

## 🎯 Comparação

Ambas as implementações:
- ✅ Mesmo algoritmo do Jogo da Vida
- ✅ Mesma seed aleatória (42)
- ✅ Wrap-around toroidal
- ✅ Mesmos parâmetros de entrada
- ✅ Formato de saída compatível

## 📝 Notas

- MPI pode precisar `--oversubscribe` para muitos processos
- Janelas requer números específicos de workers (quadrados perfeitos)
- Resultados salvos automaticamente em CSV para análise
- Tempos medidos internamente pelos programas (mais precisos)