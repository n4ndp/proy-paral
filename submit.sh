#!/bin/bash
#SBATCH --job-name=ranking_sort
#SBATCH --partition=standard
#SBATCH --nodes=1
#SBATCH --ntasks=32
#SBATCH --cpus-per-task=1
#SBATCH --time=01:00:00
#SBATCH --output=slurm_%j.log

echo "=========================================="
echo "Iniciando Job en el nodo: $HOSTNAME"
echo "CPUs asignados: $SLURM_NTASKS"
echo "=========================================="

# 1. Limpieza total
module purge

# 2. Cargar módulos
module load gnu9
module load openmpi4

# --- LA SOLUCIÓN DEFINITIVA DE ENTORNOS ---
# Forzamos las rutas de inclusión estándar de Linux
export CPATH="/usr/include:/usr/include/x86_64-linux-gnu:$CPATH"
export C_INCLUDE_PATH="/usr/include:/usr/include/x86_64-linux-gnu:$C_INCLUDE_PATH"
export CPLUS_INCLUDE_PATH="/usr/include:/usr/include/x86_64-linux-gnu:$CPLUS_INCLUDE_PATH"
# ------------------------------------------

# 3. Configuración MPI
export OMPI_MCA_rmaps_base_oversubscribe=1

# 4. Compilación y Ejecución
echo "--- Entorno Configurado ---"
echo "CXX usado por Makefile: /usr/bin/g++"
echo "MPICXX usado: $(which mpic++)"

echo "--- Iniciando Make ---"
make clean
make run-all

echo "=========================================="
echo "Job Finalizado. Revisa out.txt"
echo "=========================================="