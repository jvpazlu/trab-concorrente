#!/bin/bash

GERACOES=100
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
TAMANHOS=(512 1024 2048 4096)
WORKERS=(1 2 4 8 16 24 32 48 64)

echo "Compilando pthread..."
gcc -o game_of_life game_of_life.c -lpthread -lm -O3
if [ $? -ne 0 ]; then
    echo "Erro na compilação pthread"
    exit 1
fi

ARQUIVO_RESULTADOS="resultados/resultados_pthread_${TIMESTAMP}.csv"
echo "implementacao,tamanho,workers,tempo_ms" > $ARQUIVO_RESULTADOS

total_testes=$((${#TAMANHOS[@]} * ${#WORKERS[@]}))
teste_atual=0
sucessos=0

for tamanho in "${TAMANHOS[@]}"; do
    for threads in "${WORKERS[@]}"; do
        teste_atual=$((teste_atual + 1))
        printf "[%2d/%2d] pthread %dx%d %dt... " $teste_atual $total_testes $tamanho $tamanho $threads

        start_time=$(date +%s.%N)
        ./game_of_life faixas $threads $tamanho $GERACOES > /dev/null 2>&1
        result=$?
        end_time=$(date +%s.%N)

        if [ $result -eq 0 ]; then
            execution_time=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "0")
            tempo=$(echo "$execution_time * 1000" | bc 2>/dev/null | cut -d'.' -f1 || echo "0")

            echo "OK ${tempo}ms"
            echo "pthread,$tamanho,$threads,$tempo" >> $ARQUIVO_RESULTADOS
            sucessos=$((sucessos + 1))
        else
            echo "ERRO"
        fi
    done
done

echo ""
echo "Testes concluídos: $sucessos/$total_testes sucessos"
echo "Resultados em: $ARQUIVO_RESULTADOS"
