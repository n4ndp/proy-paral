#!/bin/bash

N=5040000
MIN=1
MAX=100
PROCESSES=(4 9 16 25 36 49 64)

OUTPUT_DIR="resultados_fijo"
mkdir -p $OUTPUT_DIR

echo "Ejecutando con N=$N fijo"
echo "P = 1, ${PROCESSES[@]}"
echo ""

echo "Ejecutando P=1"
./sequential $N $MIN $MAX | tee $OUTPUT_DIR/P01.txt
echo ""

for P in "${PROCESSES[@]}"; do
    echo "Ejecutando P=$P"
    mpirun -np $P ./v3 $N $MIN $MAX | tee $OUTPUT_DIR/P$(printf "%02d" $P).txt
    echo ""
done

CSV_FILE="$OUTPUT_DIR/resumen.csv"
echo "P,p,N,n_per_p,total_time,compute_time,comm_time,speedup,efficiency,gflops" > $CSV_FILE

T1=$(grep "^1," $OUTPUT_DIR/P01.txt | cut -d',' -f3)

LINE=$(grep "^1," $OUTPUT_DIR/P01.txt)
GFLOPS=$(echo "$LINE" | cut -d',' -f7)
echo "1,1,$N,$N,$T1,0,0,1.00,1.00,$GFLOPS" >> $CSV_FILE

for P in "${PROCESSES[@]}"; do
    FILE="$OUTPUT_DIR/P$(printf "%02d" $P).txt"
    if [ -f "$FILE" ]; then
        LINE=$(grep "^$P," $FILE)
        if [ -n "$LINE" ]; then
            p=$(echo "sqrt($P)" | bc)
            n_per_p=$((N / p))
            TP=$(echo "$LINE" | cut -d',' -f3)
            COMP=$(echo "$LINE" | cut -d',' -f4)
            COMM=$(echo "$LINE" | cut -d',' -f5)
            GFLOPS=$(echo "$LINE" | cut -d',' -f8)
            SPEEDUP=$(echo "scale=4; $T1 / $TP" | bc)
            EFFICIENCY=$(echo "scale=4; $SPEEDUP / $P" | bc)
            echo "$P,$p,$N,$n_per_p,$TP,$COMP,$COMM,$SPEEDUP,$EFFICIENCY,$GFLOPS" >> $CSV_FILE
        fi
    fi
done

echo "Resultados en: $OUTPUT_DIR/"
echo "CSV: $CSV_FILE"
