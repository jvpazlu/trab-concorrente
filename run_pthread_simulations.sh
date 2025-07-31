#!/bin/bash

# Script para executar simulações pthread com faixas e janelas
# Usa os mesmos parâmetros para comparação com MPI

gcc -o game_of_life game_of_life.c -lm -lpthread > /dev/null 2>&1

if [ ! -f "./game_of_life" ]; then
    echo "Erro: Compilação falhou"
    exit 1
fi

if [ ! -f "resultados_pthread.csv" ]; then
    echo "modo,num_threads,tamanho,num_geracoes,tempo_ms" > resultados_pthread.csv
fi

geracoes=100

tamanhos=(256 512 1024 2048 4096 8192 16384)

threads=(1 4 9 16 25 36 49 64 81 100)

total_runs=$((${#tamanhos[@]} * ${#threads[@]} * 2))  # 2 modos
current_run=0

echo "=== SIMULAÇÕES PTHREAD - MODOS FAIXAS E JANELAS ==="
echo "Executando $total_runs simulações..."

for modo in "faixas" "janelas"; do
  for tamanho in "${tamanhos[@]}"; do
    for num_threads in "${threads[@]}"; do
      current_run=$((current_run + 1))
      printf "[$current_run/$total_runs] %s %dt %dx%d... " "$modo" "$num_threads" "$tamanho" "$tamanho"

      if [ "$modo" = "janelas" ]; then
        # Para modo janelas, verifica se o número de threads é um quadrado perfeito
        raiz=$(echo "sqrt($num_threads)" | bc -l | cut -d'.' -f1)
        if [ $((raiz * raiz)) -eq $num_threads ]; then
          timeout 300 ./game_of_life $modo $num_threads $tamanho $geracoes > /dev/null 2>&1
        else
          echo "SKIP (não é quadrado perfeito)"
          echo "$modo,$num_threads,$tamanho,$geracoes,SKIP" >> resultados_pthread.csv
          continue
        fi
      else
        timeout 300 ./game_of_life $modo $num_threads $tamanho $geracoes > /dev/null 2>&1
      fi

      if [ $? -ne 0 ]; then
        echo "ERRO"
        echo "$modo,$num_threads,$tamanho,$geracoes,ERROR" >> resultados_pthread.csv
      else
        echo "OK"
      fi
    done
    sleep 18
  done
  sleep 120
done

echo "Simulações PTHREAD concluídas! Resultados em: resultados_pthread.csv"
