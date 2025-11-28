#!/bin/bash

N=(320000 640000 960000 1280000 1600000 1920000 2240000 2560000)
MIN=1
MAX=100
PROCESSES=(4 9 16 25 36 49 64)

OUTPUT_DIR="resultados_variable"
mkdir -p $OUTPUT_DIR

echo "Ejecutando con N variable (n/p = 320000 constante)"
echo "P = 1, ${PROCESSES[@]}"
echo ""

echo "Ejecutando P=1, N=${N[0]}"
./sequential ${N[0]} $MIN $MAX | tee $OUTPUT_DIR/P01.txt
echo ""

for i in "${!PROCESSES[@]}"; do
    P=${PROCESSES[$i]}
    N_val=${N[$((i+1))]}
    echo "Ejecutando P=$P, N=$N_val"
    mpirun -np $P ./v3 $N_val $MIN $MAX | tee $OUTPUT_DIR/P$(printf "%02d" $P).txt
    echo ""
done

CSV_FILE="$OUTPUT_DIR/resumen.csv"
echo "P,p,N,n_per_p,total_time,compute_time,comm_time,efficiency,gflops,scaling_factor" > $CSV_FILE

T1=$(grep "^1," $OUTPUT_DIR/P01.txt | cut -d',' -f3)

LINE=$(grep "^1," $OUTPUT_DIR/P01.txt)
GFLOPS=$(echo "$LINE" | cut -d',' -f7)
echo "1,1,${N[0]},320000,$T1,0,0,1.00,$GFLOPS,1.00" >> $CSV_FILE

for i in "${!PROCESSES[@]}"; do
    P=${PROCESSES[$i]}
    N_val=${N[$((i+1))]}
    FILE="$OUTPUT_DIR/P$(printf "%02d" $P).txt"
    if [ -f "$FILE" ]; then
        LINE=$(grep "^$P," $FILE)
        if [ -n "$LINE" ]; then
            p=$(echo "sqrt($P)" | bc)
            TP=$(echo "$LINE" | cut -d',' -f3)
            COMP=$(echo "$LINE" | cut -d',' -f4)
            COMM=$(echo "$LINE" | cut -d',' -f5)
            GFLOPS=$(echo "$LINE" | cut -d',' -f8)
            EFFICIENCY=$(echo "scale=4; $T1 / $TP" | bc)
            SCALING=$(echo "scale=4; $TP / $T1" | bc)
            echo "$P,$p,$N_val,320000,$TP,$COMP,$COMM,$EFFICIENCY,$GFLOPS,$SCALING" >> $CSV_FILE
        fi
    fi
done

echo "Resultados en: $OUTPUT_DIR/"
echo "CSV: $CSV_FILE"
