CXX = g++
MPICXX = mpic++
CXXFLAGS = -std=c++17 -O3 -Wall
LIBS = -lm

SEQ = sequential
PAR = ranking_sort_parallel

N_STRONG ?= 705600
MIN ?= 1
MAX ?= 2000000

all: $(SEQ) $(PAR)

$(SEQ): sequential.cpp
	$(CXX) $(CXXFLAGS) -o $(SEQ) sequential.cpp $(LIBS)

$(PAR): ranking_sort_parallel_FINAL.cpp
	$(MPICXX) $(CXXFLAGS) -o $(PAR) ranking_sort_parallel_FINAL.cpp $(LIBS)

clean:
	rm -f $(SEQ) $(PAR) out.txt

# ============================================================
# EXPERIMENTO 1: ESCALABILIDAD FUERTE
# ============================================================
strong: $(SEQ) $(PAR)
	@echo "========================================================================" >> out.txt
	@echo "EXPERIMENTO 1: ESCALABILIDAD FUERTE (Strong Scaling)" >> out.txt
	@echo "========================================================================" >> out.txt
	@echo "" >> out.txt
	@echo "--- 1. Ejecutando Secuencial (Base) ---" >> out.txt
	@./$(SEQ) $(N_STRONG) $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) $(N_STRONG) $(MIN) $(MAX) --time-only); \
	echo "Tiempo Base (Ts) capturado: $$TS s" >> out.txt; \
	echo "" >> out.txt; \
	echo "--- 2. Ejecutando Paralelo (P variable) ---" >> out.txt; \
	for P in 1 4 9 16 25 36 49 64; do \
		echo ">>> P = $$P <<<" >> out.txt; \
		mpirun -np $$P ./$(PAR) $$TS $(N_STRONG) $(MIN) $(MAX) >> out.txt 2>&1; \
		echo "" >> out.txt; \
	done
	@echo "Fuerte completado. Resultados en out.txt"

# ============================================================
# EXPERIMENTO 2: ESCALABILIDAD DÉBIL
# ============================================================
weak: $(SEQ) $(PAR)
	@echo "========================================================================" >> out.txt
	@echo "EXPERIMENTO 2: ESCALABILIDAD DÉBIL (Weak Scaling)" >> out.txt
	@echo "========================================================================" >> out.txt
	@echo "" >> out.txt
	
	@# P=1, N=25000
	@echo ">>> CASO P=1 (N=25000) <<<" >> out.txt
	@./$(SEQ) 25000 $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) 25000 $(MIN) $(MAX) --time-only); \
	mpirun -np 1 ./$(PAR) $$TS 25000 $(MIN) $(MAX) >> out.txt 2>&1
	@echo "" >> out.txt

	@# P=4, N=100000
	@echo ">>> CASO P=4 (N=100000) <<<" >> out.txt
	@./$(SEQ) 100000 $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) 100000 $(MIN) $(MAX) --time-only); \
	mpirun -np 4 ./$(PAR) $$TS 100000 $(MIN) $(MAX) >> out.txt 2>&1
	@echo "" >> out.txt

	@# P=9, N=225000
	@echo ">>> CASO P=9 (N=225000) <<<" >> out.txt
	@./$(SEQ) 225000 $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) 225000 $(MIN) $(MAX) --time-only); \
	mpirun -np 9 ./$(PAR) $$TS 225000 $(MIN) $(MAX) >> out.txt 2>&1
	@echo "" >> out.txt

	@# P=16, N=400000
	@echo ">>> CASO P=16 (N=400000) <<<" >> out.txt
	@./$(SEQ) 400000 $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) 400000 $(MIN) $(MAX) --time-only); \
	mpirun -np 16 ./$(PAR) $$TS 400000 $(MIN) $(MAX) >> out.txt 2>&1
	@echo "" >> out.txt

	@# P=25, N=625000
	@echo ">>> CASO P=25 (N=625000) <<<" >> out.txt
	@./$(SEQ) 625000 $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) 625000 $(MIN) $(MAX) --time-only); \
	mpirun -np 25 ./$(PAR) $$TS 625000 $(MIN) $(MAX) >> out.txt 2>&1
	@echo "" >> out.txt

	@# P=36, N=900000
	@echo ">>> CASO P=36 (N=900000) <<<" >> out.txt
	@./$(SEQ) 900000 $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) 900000 $(MIN) $(MAX) --time-only); \
	mpirun -np 36 ./$(PAR) $$TS 900000 $(MIN) $(MAX) >> out.txt 2>&1
	@echo "" >> out.txt

	@# P=49, N=1225000
	@echo ">>> CASO P=49 (N=1225000) <<<" >> out.txt
	@./$(SEQ) 1225000 $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) 1225000 $(MIN) $(MAX) --time-only); \
	mpirun -np 49 ./$(PAR) $$TS 1225000 $(MIN) $(MAX) >> out.txt 2>&1
	@echo "" >> out.txt

	@# P=64, N=1600000
	@echo ">>> CASO P=64 (N=1600000) <<<" >> out.txt
	@./$(SEQ) 1600000 $(MIN) $(MAX) >> out.txt 2>&1
	@TS=$$(./$(SEQ) 1600000 $(MIN) $(MAX) --time-only); \
	mpirun -np 64 ./$(PAR) $$TS 1600000 $(MIN) $(MAX) >> out.txt 2>&1
	
	@echo "Débil completado. Resultados agregados a out.txt"

# ============================================================
# TODO EN UNO
# ============================================================
run-all: clean all strong weak
	@echo "------------------------------------------------------"
	@echo "¡LISTO!"
	@echo "------------------------------------------------------"