#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <random>
#include <cstdlib>
#include <string>

using namespace std;

// ===== ESTRUCTURAS =====
struct Metrics {
    double total_time;
    double phase1_time;
    double phase2_time;
    double phase3_time;
    double phase4_time;
    double phase5_time;
    double compute_time;  // sort + ranking
    double comm_time;     // broadcast + reduce
};

// ===== FUNCIONES AUXILIARES =====
pair<int, int> rank_to_position(int rank, int p) {
    return {rank / p, rank % p};
}

bool is_diagonal(int rank, int p) {
    auto [row, col] = rank_to_position(rank, p);
    return row == col;
}

vector<int> generate_random_array(int N, int min_val, int max_val, int seed = 42) {
    vector<int> data;
    data.reserve(N);
    
    mt19937 rng(seed);
    uniform_int_distribution<int> dist(min_val, max_val);
    
    for (int i = 0; i < N; i++) {
        data.push_back(dist(rng));
    }
    
    return data;
}

// ===== FASE 1: INPUT + GOSSIP (DISTRIBUCIÓN LOCAL) =====
// Cada proceso genera los datos completos y selecciona lo que necesita
vector<int> phase1_input_gossip(
    const vector<int>& global_data,
    int rank, int size, int p
) {
    int N = global_data.size();
    int P = size;
    int elements_per_group = N / P;        // N/P
    int elements_per_process = elements_per_group * p;  // N/p
    
    vector<int> local_data;
    local_data.reserve(elements_per_process);
    
    int rank_mod_p = rank % p;
    
    // Seleccionar grupos según rank_mod_p
    for (int group_id = 0; group_id < P; group_id++) {
        if (group_id % p == rank_mod_p) {
            int start_idx = group_id * elements_per_group;
            for (int i = 0; i < elements_per_group; i++) {
                local_data.push_back(global_data[start_idx + i]);
            }
        }
    }
    
    return local_data;
}

// ===== FASE 2: BROADCAST HORIZONTAL =====
vector<int> phase2_broadcast(
    const vector<int>& local_data,
    int rank, int p,
    MPI_Comm row_comm
) {
    auto [row, col] = rank_to_position(rank, p);
    
    vector<int> broadcasted_data(local_data.size());
    
    // Procesos diagonales copian sus datos
    if (is_diagonal(rank, p)) {
        broadcasted_data = local_data;
    }
    
    // Broadcast dentro de cada fila
    MPI_Bcast(broadcasted_data.data(), broadcasted_data.size(), MPI_INT, row, row_comm);
    
    return broadcasted_data;
}

// ===== FASE 3: SORT LOCAL =====
void phase3_sort(vector<int>& local_data) {
    sort(local_data.begin(), local_data.end());
}

// ===== FASE 4: LOCAL RANKING =====
vector<int> phase4_local_ranking(
    const vector<int>& sorted_local, 
    const vector<int>& broadcasted
) {
    vector<int> ranking;
    ranking.reserve(broadcasted.size());
    
    for (int value : broadcasted) {
        int count = upper_bound(sorted_local.begin(), sorted_local.end(), value) 
                    - sorted_local.begin();
        ranking.push_back(count);
    }
    
    return ranking;
}

// ===== FASE 5: REDUCE HORIZONTAL =====
vector<int> phase5_reduce(
    const vector<int>& local_ranking,
    int rank, int p,
    MPI_Comm row_comm
) {
    auto [row, col] = rank_to_position(rank, p);
    
    vector<int> reduced_ranking(local_ranking.size());
    
    MPI_Reduce(
        local_ranking.data(), reduced_ranking.data(), local_ranking.size(),
        MPI_INT, MPI_SUM, row, row_comm
    );
    
    return reduced_ranking;
}

// ===== CÁLCULO DE FLOPs =====
long long calculate_flops(int N, int p) {
    // Trabajo por proceso
    int n = N / p;  // Elementos por proceso
    
    // Sort: n log(n) comparaciones/swaps
    long long sort_ops = (long long)n * (long long)log2(n);
    
    // Ranking: n búsquedas binarias, cada una O(log n)
    long long ranking_ops = (long long)n * (long long)log2(n);
    
    // Total por proceso
    long long ops_per_process = sort_ops + ranking_ops;
    
    // Total en el sistema (P procesos)
    long long total_ops = ops_per_process * (p * p);
    
    return total_ops;
}

// ===== IMPRESIÓN DE MÉTRICAS =====
void print_metrics(int rank, int size, int N, int p, const Metrics& m, double Ts, bool verbose) {
    if (rank == 0) {
        cout << "\n" << string(70, '=') << "\n";
        cout << "RANKING SORT PARALELO - MÉTRICAS DE PERFORMANCE\n";
        cout << string(70, '=') << "\n";
        
        cout << fixed << setprecision(3);
        
        // Configuración
        cout << "Configuración:\n";
        cout << "  N (elementos):     " << N << "\n";
        cout << "  P (procesos):      " << size << " (malla " << p << "×" << p << ")\n";
        cout << "  Elementos/proceso: " << (N/p) << "\n\n";
        
        // Tiempos
        cout << "Tiempos:\n";
        cout << "  Total (Tp):        " << (m.total_time * 1000) << " ms\n";
        cout << "  - Cómputo:         " << (m.compute_time * 1000) << " ms ("
             << (m.compute_time/m.total_time*100) << "%)\n";
        cout << "  - Comunicación:    " << (m.comm_time * 1000) << " ms ("
             << (m.comm_time/m.total_time*100) << "%)\n";
        
        if (verbose) {
            cout << "\n  Desglose detallado:\n";
            cout << "    Fase 1 (Input):    " << (m.phase1_time * 1000) << " ms\n";
            cout << "    Fase 2 (Bcast):    " << (m.phase2_time * 1000) << " ms\n";
            cout << "    Fase 3 (Sort):     " << (m.phase3_time * 1000) << " ms\n";
            cout << "    Fase 4 (Ranking):  " << (m.phase4_time * 1000) << " ms\n";
            cout << "    Fase 5 (Reduce):   " << (m.phase5_time * 1000) << " ms\n";
        }
        
        // Rendimiento (si Ts disponible)
        if (Ts > 0) {
            double speedup = Ts / m.total_time;
            double efficiency = speedup / size;
            double speedup_ideal = size;
            
            cout << "\nRendimiento:\n";
            cout << "  Ts (secuencial):   " << (Ts * 1000) << " ms\n";
            cout << "  Speedup (S):       " << speedup << "x\n";
            cout << "  Eficiencia (E):    " << (efficiency * 100) << "%\n\n";
            
            cout << "  Speedup ideal:     " << speedup_ideal << "x\n";
            cout << "  % del ideal:       " << (speedup/speedup_ideal*100) << "%\n";
        }
        
        // FLOPs
        long long flops = calculate_flops(N, p);
        double flops_per_sec = flops / m.compute_time;  // Usar solo tiempo de cómputo
        double gflops = flops_per_sec / 1e9;
        double mflops = flops_per_sec / 1e6;
        
        cout << "\nFLOPs:\n";
        cout << "  Operaciones:       " << flops << "\n";
        cout << "  FLOP/s:            " << flops_per_sec << "\n";
        cout << "  MFLOP/s:           " << mflops << "\n";
        cout << "  GFLOP/s:           " << gflops << "\n";
        
        // Throughput
        double throughput = N / m.total_time;
        cout << "\nThroughput:\n";
        cout << "  Elementos/seg:     " << throughput << "\n";
        
        // Balance
        if (m.comm_time > 0) {
            cout << "\nBalance:\n";
            cout << "  Cómputo/Comm:      " << (m.compute_time/m.comm_time) << "x\n";
        }
        
        cout << string(70, '=') << "\n";
        
        // CSV para análisis
        cout << "\nFORMATO CSV:\n";
        cout << "P,N,p,Tp_ms,compute_ms,comm_ms,";
        if (Ts > 0) cout << "Ts_ms,speedup,efficiency,";
        cout << "flops,gflops,throughput\n";
        
        cout << size << "," << N << "," << p << ","
             << (m.total_time*1000) << ","
             << (m.compute_time*1000) << ","
             << (m.comm_time*1000) << ",";
        
        if (Ts > 0) {
            cout << (Ts*1000) << ","
                 << (Ts/m.total_time) << ","
                 << ((Ts/m.total_time)/size) << ",";
        }
        
        cout << flops << ","
             << gflops << ","
             << throughput << "\n";
    }
}

// ===== IMPRESIÓN DE RESULTADOS POR PROCESO =====
void print_process_data(
    int rank, int p,
    const vector<int>& original,
    const vector<int>& sorted_local,
    const vector<int>& broadcasted,
    const vector<int>& local_ranking,
    const vector<int>& reduced_ranking
) {
    auto [row, col] = rank_to_position(rank, p);
    bool is_diag = is_diagonal(rank, p);
    
    cout << "\n" << string(70, '-') << "\n";
    cout << "Proceso " << rank << " (fila=" << row << ", col=" << col << ")";
    if (is_diag) cout << " [DIAGONAL]";
    cout << "\n" << string(70, '-') << "\n";
    
    int show = min(12, (int)original.size());
    
    cout << "Original:       [";
    for (int i = 0; i < show; i++) {
        cout << setw(4) << original[i];
        if (i < show - 1) cout << ",";
    }
    if (original.size() > (size_t)show) cout << ", ...";
    cout << "]\n";
    
    cout << "Sorted:         [";
    for (int i = 0; i < show; i++) {
        cout << setw(4) << sorted_local[i];
        if (i < show - 1) cout << ",";
    }
    if (sorted_local.size() > (size_t)show) cout << ", ...";
    cout << "]\n";
    
    cout << "Broadcasted:    [";
    for (int i = 0; i < show; i++) {
        cout << setw(4) << broadcasted[i];
        if (i < show - 1) cout << ",";
    }
    if (broadcasted.size() > (size_t)show) cout << ", ...";
    cout << "]\n";
    
    cout << "Local Ranking:  [";
    for (int i = 0; i < show; i++) {
        cout << setw(4) << local_ranking[i];
        if (i < show - 1) cout << ",";
    }
    if (local_ranking.size() > (size_t)show) cout << ", ...";
    cout << "]\n";
    
    if (is_diag) {
        cout << "Global Ranking: [";
        for (int i = 0; i < show; i++) {
            cout << setw(4) << reduced_ranking[i];
            if (i < show - 1) cout << ",";
        }
        if (reduced_ranking.size() > (size_t)show) cout << ", ...";
        cout << "]\n";
    }
}

// ===== MAIN =====
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Verificar argumentos
    if (argc < 4) {
        if (rank == 0) {
            cerr << "Uso: mpirun -np P " << argv[0] << " [Ts] <N> <min> <max> [opciones]\n";
            cerr << "\nArgumentos:\n";
            cerr << "  Ts:  Tiempo secuencial en SEGUNDOS (opcional, para calcular speedup)\n";
            cerr << "  N:   Número de elementos\n";
            cerr << "  min: Valor mínimo aleatorio\n";
            cerr << "  max: Valor máximo aleatorio\n";
            cerr << "\nOpciones:\n";
            cerr << "  -v, --verbose   Desglose detallado de tiempos por fase\n";
            cerr << "  -r, --results   Mostrar datos de cada proceso\n";
            cerr << "\nEjemplos:\n";
            cerr << "  # Sin speedup:\n";
            cerr << "  mpirun -np 4 " << argv[0] << " 1000 1 100\n";
            cerr << "  mpirun -np 4 " << argv[0] << " 1000 1 100 -v\n\n";
            cerr << "  # Con speedup (pasar Ts en segundos):\n";
            cerr << "  Ts=$(./sequential 1000 1 100 --time-only)\n";
            cerr << "  mpirun -np 4 " << argv[0] << " $Ts 1000 1 100 -v\n\n";
            cerr << "  # Con resultados detallados:\n";
            cerr << "  mpirun -np 4 " << argv[0] << " 0.5 1000 1 100 -v -r\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    // Parsear Ts (opcional)
    double Ts = -1;
    int arg_offset = 1;
    
    char* endptr;
    double first_arg = strtod(argv[1], &endptr);
    if (*endptr == '\0' && first_arg > 0 && first_arg < 1000) {
        // Es un número razonable, probablemente Ts
        Ts = first_arg;
        arg_offset = 2;
    }
    
    if (argc < arg_offset + 3) {
        if (rank == 0) cerr << "ERROR: Argumentos insuficientes\n";
        MPI_Finalize();
        return 1;
    }
    
    // Parsear parámetros obligatorios
    int N = atoi(argv[arg_offset]);
    int min_val = atoi(argv[arg_offset + 1]);
    int max_val = atoi(argv[arg_offset + 2]);
    
    // Parsear opciones
    bool verbose = false;
    bool show_results = false;
    
    for (int i = arg_offset + 3; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") verbose = true;
        if (arg == "-r" || arg == "--results") show_results = true;
    }
    
    // Validaciones
    if (N <= 0) {
        if (rank == 0) cerr << "ERROR: N debe ser positivo\n";
        MPI_Finalize();
        return 1;
    }
    
    if (min_val >= max_val) {
        if (rank == 0) cerr << "ERROR: min debe ser menor que max\n";
        MPI_Finalize();
        return 1;
    }
    
    int p = static_cast<int>(sqrt(size));
    if (p * p != size) {
        if (rank == 0) {
            cerr << "ERROR: P debe ser cuadrado perfecto (P = p²)\n";
            cerr << "Recibido: P = " << size << "\n";
            cerr << "Valores válidos: 1, 4, 9, 16, 25, 36, 49, 64, ...\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    if (N % size != 0) {
        if (rank == 0) {
            cerr << "ERROR: N debe ser divisible por P\n";
            cerr << "N = " << N << ", P = " << size << "\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    // TODOS los procesos generan los mismos datos (semilla fija)
    vector<int> global_data = generate_random_array(N, min_val, max_val);
    
    // Crear comunicador de fila
    auto [row, col] = rank_to_position(rank, p);
    MPI_Comm row_comm;
    MPI_Comm_split(MPI_COMM_WORLD, row, col, &row_comm);
    
    // Inicializar métricas
    Metrics metrics = {0};
    double t_start;
    
    // ===== EJECUCIÓN DEL ALGORITMO =====
    MPI_Barrier(MPI_COMM_WORLD);
    double total_start = MPI_Wtime();
    
    // FASE 1: Input + Gossip
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    vector<int> local_data = phase1_input_gossip(global_data, rank, size, p);
    vector<int> original_data = local_data;  // Guardar copia para -r
    MPI_Barrier(MPI_COMM_WORLD);
    metrics.phase1_time = MPI_Wtime() - t_start;
    
    // FASE 2: Broadcast
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    vector<int> broadcasted_data = phase2_broadcast(local_data, rank, p, row_comm);
    MPI_Barrier(MPI_COMM_WORLD);
    metrics.phase2_time = MPI_Wtime() - t_start;
    
    // FASE 3: Sort
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    phase3_sort(local_data);
    MPI_Barrier(MPI_COMM_WORLD);
    metrics.phase3_time = MPI_Wtime() - t_start;
    
    // FASE 4: Ranking
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    vector<int> local_ranking = phase4_local_ranking(local_data, broadcasted_data);
    MPI_Barrier(MPI_COMM_WORLD);
    metrics.phase4_time = MPI_Wtime() - t_start;
    
    // FASE 5: Reduce
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    vector<int> reduced_ranking = phase5_reduce(local_ranking, rank, p, row_comm);
    MPI_Barrier(MPI_COMM_WORLD);
    metrics.phase5_time = MPI_Wtime() - t_start;
    
    // Tiempo total
    MPI_Barrier(MPI_COMM_WORLD);
    metrics.total_time = MPI_Wtime() - total_start;
    
    // Calcular tiempos agregados
    metrics.compute_time = metrics.phase3_time + metrics.phase4_time;
    metrics.comm_time = metrics.phase2_time + metrics.phase5_time;
    
    // ===== SALIDA =====
    print_metrics(rank, size, N, p, metrics, Ts, verbose);
    
    if (show_results) {
        for (int i = 0; i < size; i++) {
            if (rank == i) {
                print_process_data(rank, p, original_data, local_data,
                                  broadcasted_data, local_ranking, reduced_ranking);
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    
    // Cleanup
    MPI_Comm_free(&row_comm);
    MPI_Finalize();
    return 0;
}