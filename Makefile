SEQ = ./sequential
PAR = ./ranking_sort_parallel

MIN ?= 1
MAX ?= 20000000

OUT = out.txt

clean:
	rm -f $(OUT)

# ============================================================
# EXPERIMENTO 1: STRONG SCALING
# ============================================================
# Lista de Ns fijos a probar:
# 705,600 | 1,058,400 | 1,411,200 | 1,764,000 | 2,116,800 | 2,469,600 | 2,822,400
NS_STRONG = 705600 1058400 1411200 1764000 2116800 2469600 2822400

strong:
	@echo "========================================================================" >> $(OUT)
	@echo "     INICIANDO BATERÍA DE PRUEBAS STRONG SCALING" >> $(OUT)
	@echo "========================================================================" >> $(OUT)
	@echo "" >> $(OUT)
	
	@# Bucle para cada N fijo
	@for N in $(NS_STRONG); do \
		echo "========================================================================" >> $(OUT); \
		echo ">>> STRONG SCALING CON N FIJO = $$N <<<" >> $(OUT); \
		echo "========================================================================" >> $(OUT); \
		\
		echo "--- 1. Ejecutando Secuencial (Base) ---" >> $(OUT); \
		$(SEQ) $$N $(MIN) $(MAX) >> $(OUT) 2>&1; \
		TS=$$($(SEQ) $$N $(MIN) $(MAX) --time-only); \
		echo "Tiempo Base (Ts) capturado: $$TS s" >> $(OUT); \
		echo "" >> $(OUT); \
		\
		echo "--- 2. Ejecutando Paralelo (P variable) ---" >> $(OUT); \
		for P in 1 4 9 16 25 36 49 64; do \
			echo "   -> Ejecutando P=$$P..." >> $(OUT); \
			mpirun -np $$P $(PAR) $$TS $$N $(MIN) $(MAX) >> $(OUT) 2>&1; \
		done; \
		echo "" >> $(OUT); \
	done
	@echo ">>> TODAS LAS PRUEBAS DE STRONG SCALING COMPLETADAS <<<"

# ============================================================
# EXPERIMENTO 2: WEAK SCALING
# Regla: N = 352,800 * sqrt(P)
# ============================================================
weak:
	@echo "========================================================================" >> $(OUT)
	@echo "     WEAK SCALING" >> $(OUT)
	@echo "     Regla: N = 352,800 * sqrt(P)" >> $(OUT)
	@echo "========================================================================" >> $(OUT)
	@echo "" >> $(OUT)
	
	@# P=1 | N = 352,800
	@echo ">>> CASO P=1 (N=352800) <<<" >> $(OUT)
	@$(SEQ) 352800 $(MIN) $(MAX) >> $(OUT) 2>&1
	@TS=$$($(SEQ) 352800 $(MIN) $(MAX) --time-only); \
	mpirun -np 1 $(PAR) $$TS 352800 $(MIN) $(MAX) >> $(OUT) 2>&1
	@echo "" >> $(OUT)

	@# P=4 | N = 705,600
	@echo ">>> CASO P=4 (N=705600) <<<" >> $(OUT)
	@$(SEQ) 705600 $(MIN) $(MAX) >> $(OUT) 2>&1
	@TS=$$($(SEQ) 705600 $(MIN) $(MAX) --time-only); \
	mpirun -np 4 $(PAR) $$TS 705600 $(MIN) $(MAX) >> $(OUT) 2>&1
	@echo "" >> $(OUT)

	@# P=9 | N = 1,058,400
	@echo ">>> CASO P=9 (N=1058400) <<<" >> $(OUT)
	@$(SEQ) 1058400 $(MIN) $(MAX) >> $(OUT) 2>&1
	@TS=$$($(SEQ) 1058400 $(MIN) $(MAX) --time-only); \
	mpirun -np 9 $(PAR) $$TS 1058400 $(MIN) $(MAX) >> $(OUT) 2>&1
	@echo "" >> $(OUT)

	@# P=16 | N = 1,411,200
	@echo ">>> CASO P=16 (N=1411200) <<<" >> $(OUT)
	@$(SEQ) 1411200 $(MIN) $(MAX) >> $(OUT) 2>&1
	@TS=$$($(SEQ) 1411200 $(MIN) $(MAX) --time-only); \
	mpirun -np 16 $(PAR) $$TS 1411200 $(MIN) $(MAX) >> $(OUT) 2>&1
	@echo "" >> $(OUT)

	@# P=25 | N = 1,764,000
	@echo ">>> CASO P=25 (N=1764000) <<<" >> $(OUT)
	@$(SEQ) 1764000 $(MIN) $(MAX) >> $(OUT) 2>&1
	@TS=$$($(SEQ) 1764000 $(MIN) $(MAX) --time-only); \
	mpirun -np 25 $(PAR) $$TS 1764000 $(MIN) $(MAX) >> $(OUT) 2>&1
	@echo "" >> $(OUT)

	@# P=36 | N = 2,116,800
	@echo ">>> CASO P=36 (N=2116800) <<<" >> $(OUT)
	@$(SEQ) 2116800 $(MIN) $(MAX) >> $(OUT) 2>&1
	@TS=$$($(SEQ) 2116800 $(MIN) $(MAX) --time-only); \
	mpirun -np 36 $(PAR) $$TS 2116800 $(MIN) $(MAX) >> $(OUT) 2>&1
	@echo "" >> $(OUT)

	@# P=49 | N = 2,469,600
	@echo ">>> CASO P=49 (N=2469600) <<<" >> $(OUT)
	@$(SEQ) 2469600 $(MIN) $(MAX) >> $(OUT) 2>&1
	@TS=$$($(SEQ) 2469600 $(MIN) $(MAX) --time-only); \
	mpirun -np 49 $(PAR) $$TS 2469600 $(MIN) $(MAX) >> $(OUT) 2>&1
	@echo "" >> $(OUT)

	@# P=64 | N = 2,822,400
	@echo ">>> CASO P=64 (N=2822400) <<<" >> $(OUT)
	@$(SEQ) 2822400 $(MIN) $(MAX) >> $(OUT) 2>&1
	@TS=$$($(SEQ) 2822400 $(MIN) $(MAX) --time-only); \
	mpirun -np 64 $(PAR) $$TS 2822400 $(MIN) $(MAX) >> $(OUT) 2>&1
	
	@echo ">>> WEAK SCALING COMPLETADO <<<"

# ============================================================
# EJECUCIÓN MAESTRA
# ============================================================
run-all: clean strong weak
	@echo "------------------------------------------------------"
	@echo "¡LISTO! Se han ejecutado TODAS las pruebas."
	@echo "Revisa el archivo 'out.txt' para ver los resultados."
	@echo "------------------------------------------------------"