#!/bin/bash

make clean && make > /dev/null 2>&1

if [ ! -f "./game_of_life" ]; then
    echo "Erro: Compilação falhou"
    exit 1
fi

echo "modo,num_threads,tamanho,num_geracoes,tempo_ms" > resultados.csv

geracoes=100

tamanhos=(256 512 1024 2048 4096 8192 16384)

threads=(1 4 9 16 25 36 49 64 81 100)

total_runs=$((${#tamanhos[@]} * ${#threads[@]} * 2))  # 2 modos
current_run=0

echo "Executando $total_runs simulações..."

for modo in "faixas" "janelas"; do
  for tamanho in "${tamanhos[@]}"; do
    for num_threads in "${threads[@]}"; do
      current_run=$((current_run + 1))
      printf "[$current_run/$total_runs] %s %dt %dx%d... " "$modo" "$num_threads" "$tamanho" "$tamanho"

      ./game_of_life $modo $num_threads $tamanho $geracoes > /dev/null 2>&1

      if [ $? -ne 0 ]; then
        echo "ERRO"
        echo "$modo,$num_threads,$tamanho,$geracoes,ERROR" >> resultados.csv
      else
        echo "OK"
      fi
    done
  done
done

echo "Simulações concluídas! Resultados em: resultados.csv"
