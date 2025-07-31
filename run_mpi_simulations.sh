#!/bin/bash

mpicc -o game_of_life_mpi game_of_life_mpi.c -lm > /dev/null 2>&1

if [ ! -f "./game_of_life_mpi" ]; then
    echo "Erro: Compilação falhou"
    exit 1
fi

if [ ! -f "resultados_mpi.csv" ]; then
    echo "modo,num_procs,tamanho,num_geracoes,tempo_ms" > resultados_mpi.csv
fi

geracoes=100

tamanhos=(256 512 1024 2048 4096 8192 16384)

procs=(1 4 9 16 25 36 49 64 81 100)

total_runs=$((${#tamanhos[@]} * ${#procs[@]} * 2))  # 2 modos
current_run=0

echo "=== SIMULAÇÕES MPI - MODOS FAIXAS E JANELAS ==="
echo "Executando $total_runs simulações MPI..."

for modo in "faixas" "janelas"; do
  for tamanho in "${tamanhos[@]}"; do
    for num_procs in "${procs[@]}"; do
      current_run=$((current_run + 1))
      printf "[$current_run/$total_runs] ${modo}_mpi %dp %dx%d... " "$num_procs" "$tamanho" "$tamanho"

      if [ "$modo" = "janelas" ]; then
        # Para modo janelas, verifica se o número de processos forma uma grade
        raiz=$(echo "sqrt($num_procs)" | bc -l | cut -d'.' -f1)
        if [ $((raiz * raiz)) -eq $num_procs ] || [ $num_procs -eq 1 ] || [ $num_procs -eq 4 ] || [ $num_procs -eq 9 ] || [ $num_procs -eq 16 ] || [ $num_procs -eq 25 ] || [ $num_procs -eq 36 ] || [ $num_procs -eq 49 ] || [ $num_procs -eq 64 ] || [ $num_procs -eq 81 ] || [ $num_procs -eq 100 ]; then
          timeout 300 mpirun --oversubscribe -np $num_procs ./game_of_life_mpi $modo $num_procs $tamanho $geracoes > /dev/null 2>&1
        else
          echo "SKIP (não é quadrado perfeito)"
          echo "${modo}_mpi,$num_procs,$tamanho,$geracoes,SKIP" >> resultados_mpi.csv
          continue
        fi
      else
        timeout 300 mpirun -np $num_procs ./game_of_life_mpi $modo $num_procs $tamanho $geracoes > /dev/null 2>&1
      fi

      if [ $? -ne 0 ]; then
        echo "ERRO"
        echo "${modo}_mpi,$num_procs,$tamanho,$geracoes,ERROR" >> resultados_mpi.csv
      else
        echo "OK"
      fi
    done
    sleep 18
  done
  sleep 120
done

echo "Simulações MPI concluídas! Resultados em: resultados_mpi.csv"
