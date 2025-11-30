#!/bin/bash
#SBATCH --job-name=ranking_sort
#SBATCH --partition=standard      # Usamos la partición con nodos de 64 cores
#SBATCH --nodes=1                 # Todo en un solo nodo para minimizar latencia
#SBATCH --ntasks=32               # Pedimos 32 cores (para evitar el QOS Limit)
#SBATCH --cpus-per-task=1
#SBATCH --time=00:30:00           # 30 minutos es suficiente
#SBATCH --output=slurm_%j.log     # Log de salida del sistema

echo "=========================================="
echo "Iniciando Job en el nodo: $HOSTNAME"
echo "CPUs asignados: $SLURM_NTASKS"
echo "=========================================="

# 1. Cargar módulos (IMPORTANTE: Verifica si necesitas cargar MPI)
# En Khipu usualmente es necesario, si ya compila por defecto ignora esto.
# module load openmpi/4.x  <-- Descomenta si mpic++ no se encuentra

# 2. Limpiar y Compilar
echo "--- Compilando ---"
make clean
make all

# 3. Ejecutar los experimentos
echo "--- Ejecutando Experimentos ---"
# El Makefile ya se encarga de llamar a mpirun y guardar en out.txt
# PERO, necesitamos decirle a MPI que puede usar más de 32 procesos si es necesario
# Exportamos esta variable para permitir oversubscribe sin cambiar el Makefile
export OMPI_MCA_rmaps_base_oversubscribe=1 

make run-all

echo "=========================================="
echo "Job Finalizado. Revisa out.txt"
echo "=========================================="