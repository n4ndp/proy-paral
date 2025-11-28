#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <random>
#include <cstdlib>

using namespace std;

struct TimeMetrics {
    double total_time;
    double compute_time;
    double comm_time;
    double phase1_time;
    double phase2_time;
    double phase3_time;
    double phase4_time;
    double phase5_time;
};

pair<int, int> rank_to_position(int rank, int p) {
    return {rank / p, rank % p};
}

bool is_diagonal(int rank, int p) {
    auto [row, col] = rank_to_position(rank, p);
    return row == col;
}

int get_diagonal_of_row(int row, int p) {
    return row * p + row;
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

vector<int> phase1_input_gossip(
    const vector<int>& global_data,
    int rank, int size, int p
) {
    int N = global_data.size();
    int P = size;
    int elements_per_group = N / P;
    int elements_per_process = elements_per_group * p;
    
    vector<int> local_data;
    local_data.reserve(elements_per_process);
    
    int rank_mod_p = rank % p;
    
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

vector<int> phase2_broadcast(
    const vector<int>& local_data,
    int rank, int size, int p,
    MPI_Comm row_comm
) {
    auto [row, col] = rank_to_position(rank, p);
    
    vector<int> broadcasted_data(local_data.size());
    
    if (is_diagonal(rank, p)) {
        broadcasted_data = local_data;
    }
    
    MPI_Bcast(broadcasted_data.data(), broadcasted_data.size(), MPI_INT, row, row_comm);
    
    return broadcasted_data;
}

void phase3_sort(vector<int>& local_data) {
    sort(local_data.begin(), local_data.end());
}

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

vector<int> phase5_reduce(
    const vector<int>& local_ranking,
    int rank, int size, int p,
    MPI_Comm row_comm
) {
    auto [row, col] = rank_to_position(rank, p);
    
    vector<int> reduced_ranking(local_ranking.size());
    
    MPI_Reduce(local_ranking.data(), reduced_ranking.data(), local_ranking.size(),
               MPI_INT, MPI_SUM, row, row_comm);
    
    return reduced_ranking;
}

long long calculate_flops(int N, int p) {
    int n = N / p;
    
    long long sort_ops = n * log2(n);
    long long ranking_ops = n * log2(n);
    long long ops_per_process = sort_ops + ranking_ops;
    long long total_work = ops_per_process * (p * p);
    
    return total_work;
}

void print_metrics_header() {
    cout << "\n" << string(80, '=') << "\n";
    cout << "MÉTRICAS DE RENDIMIENTO\n";
    cout << string(80, '=') << "\n";
}

void print_metrics(int rank, int size, int N, int p, const TimeMetrics& metrics, bool verbose) {
    if (rank == 0) {
        print_metrics_header();
        
        cout << fixed << setprecision(6);
        cout << "N (elementos):           " << N << "\n";
        cout << "P (procesos):            " << size << " (p = " << p << ")\n";
        cout << "Elementos por proceso:   " << N/p << "\n\n";
        
        cout << "TIEMPOS (segundos):\n";
        cout << "  Total:                 " << metrics.total_time << "\n";
        cout << "  Cómputo:               " << metrics.compute_time 
             << " (" << (metrics.compute_time/metrics.total_time*100) << "%)\n";
        cout << "  Comunicación:          " << metrics.comm_time 
             << " (" << (metrics.comm_time/metrics.total_time*100) << "%)\n";
        
        if (verbose) {
            cout << "\nDESGLOSE POR FASES:\n";
            cout << "  Fase 1 (Input+Gossip): " << metrics.phase1_time << "\n";
            cout << "  Fase 2 (Broadcast):    " << metrics.phase2_time << "\n";
            cout << "  Fase 3 (Sort):         " << metrics.phase3_time << "\n";
            cout << "  Fase 4 (Ranking):      " << metrics.phase4_time << "\n";
            cout << "  Fase 5 (Reduce):       " << metrics.phase5_time << "\n";
        }
        
        long long flops = calculate_flops(N, p);
        double flops_per_sec = flops / metrics.compute_time;
        double gflops = flops_per_sec / 1e9;
        double mflops = flops_per_sec / 1e6;
        
        cout << "\nFLOPs:\n";
        cout << "  Operaciones estimadas: " << flops << "\n";
        cout << "  Velocidad (FLOP/s):    " << flops_per_sec << "\n";
        cout << "  Velocidad (MFLOP/s):   " << mflops << "\n";
        cout << "  Velocidad (GFLOP/s):   " << gflops << "\n";
        
        double throughput = N / metrics.total_time;
        cout << "\nTHROUGHPUT:\n";
        cout << "  Elementos/segundo:     " << throughput << "\n";
        
        cout << "\nBALANCE CÓMPUTO/COMUNICACIÓN:\n";
        cout << "  Razón:                 " << (metrics.compute_time/metrics.comm_time) << "\n";
        
        cout << string(80, '=') << "\n";
        
        cout << "\nFORMATO CSV (para análisis):\n";
        cout << "P,N,total_time,compute_time,comm_time,flops,flops_per_sec,mflops,gflops,throughput\n";
        cout << size << "," << N << "," 
             << metrics.total_time << "," 
             << metrics.compute_time << "," 
             << metrics.comm_time << "," 
             << flops << ","
             << flops_per_sec << ","
             << mflops << ","
             << gflops << ","
             << throughput << "\n";
    }
}

void print_results(int rank, int p,
                   const vector<int>& original,
                   const vector<int>& sorted_local,
                   const vector<int>& broadcasted,
                   const vector<int>& local_ranking,
                   const vector<int>& reduced_ranking) {
    
    auto [row, col] = rank_to_position(rank, p);
    bool is_diag = is_diagonal(rank, p);
    
    cout << "\nProceso " << rank << " (fila=" << row << ", columna=" << col << ")";
    if (is_diag) cout << " [DIAGONAL]";
    cout << "\n";
    
    cout << "  Original:     ";
    for (size_t i = 0; i < min(original.size(), size_t(10)); i++) {
        cout << setw(4) << original[i];
        if (i < min(original.size(), size_t(10)) - 1) cout << " ";
    }
    if (original.size() > 10) cout << " ...";
    cout << "\n";
    
    cout << "  Sorted:       ";
    for (size_t i = 0; i < min(sorted_local.size(), size_t(10)); i++) {
        cout << setw(4) << sorted_local[i];
        if (i < min(sorted_local.size(), size_t(10)) - 1) cout << " ";
    }
    if (sorted_local.size() > 10) cout << " ...";
    cout << "\n";
    
    if (is_diag) {
        cout << "  Reduced Rank: ";
        for (size_t i = 0; i < min(reduced_ranking.size(), size_t(10)); i++) {
            cout << setw(4) << reduced_ranking[i];
            if (i < min(reduced_ranking.size(), size_t(10)) - 1) cout << " ";
        }
        if (reduced_ranking.size() > 10) cout << " ...";
        cout << "\n";
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    bool verbose = false;
    bool show_results = false;
    
    if (argc < 4) {
        if (rank == 0) {
            cerr << "Uso: " << argv[0] << " <N> <min> <max> [opciones]\n";
            cerr << "  N:   Número de elementos a ordenar\n";
            cerr << "  min: Valor mínimo para números aleatorios\n";
            cerr << "  max: Valor máximo para números aleatorios\n";
            cerr << "\nOpciones:\n";
            cerr << "  -v, --verbose     Mostrar desglose detallado de tiempos\n";
            cerr << "  -r, --results     Mostrar resultados de cada proceso\n";
            cerr << "\nEjemplos:\n";
            cerr << "  " << argv[0] << " 1000 1 100\n";
            cerr << "  " << argv[0] << " 1000 1 100 -v\n";
            cerr << "  " << argv[0] << " 1000 1 100 -v -r\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    int N = atoi(argv[1]);
    int min_val = atoi(argv[2]);
    int max_val = atoi(argv[3]);
    
    for (int i = 4; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") verbose = true;
        if (arg == "-r" || arg == "--results") show_results = true;
    }
    
    if (N <= 0) {
        if (rank == 0) cerr << "ERROR: N debe ser un número positivo\n";
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
            cerr << "ERROR: El número de procesos debe ser un cuadrado perfecto (P = p²)\n";
            cerr << "Recibido: " << size << " procesos\n";
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
    
    vector<int> global_data = generate_random_array(N, min_val, max_val);
    
    if (rank == 0) {
        cout << "\nORDENAMIENTO POR RANKING EN PARALELO\n";
        cout << "N = " << N << " elementos, P = " << size << " procesos (p = " << p << ")\n";
        cout << "Rango: [" << min_val << ", " << max_val << "]\n";
    }
    
    auto [row, col] = rank_to_position(rank, p);
    MPI_Comm row_comm;
    MPI_Comm_split(MPI_COMM_WORLD, row, col, &row_comm);
    
    TimeMetrics metrics = {0};
    double t_start, t_end;
    
    MPI_Barrier(MPI_COMM_WORLD);
    double total_start = MPI_Wtime();
    
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    vector<int> local_data = phase1_input_gossip(global_data, rank, size, p);
    vector<int> original_data = local_data;
    t_end = MPI_Wtime();
    metrics.phase1_time = t_end - t_start;
    
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    vector<int> broadcasted_data = phase2_broadcast(local_data, rank, size, p, row_comm);
    t_end = MPI_Wtime();
    metrics.phase2_time = t_end - t_start;
    
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    phase3_sort(local_data);
    t_end = MPI_Wtime();
    metrics.phase3_time = t_end - t_start;
    
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    vector<int> local_ranking = phase4_local_ranking(local_data, broadcasted_data);
    t_end = MPI_Wtime();
    metrics.phase4_time = t_end - t_start;
    
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    vector<int> reduced_ranking = phase5_reduce(local_ranking, rank, size, p, row_comm);
    t_end = MPI_Wtime();
    metrics.phase5_time = t_end - t_start;
    
    MPI_Barrier(MPI_COMM_WORLD);
    double total_end = MPI_Wtime();
    metrics.total_time = total_end - total_start;
    
    metrics.compute_time = metrics.phase3_time + metrics.phase4_time;
    metrics.comm_time = metrics.phase2_time + metrics.phase5_time;
    
    print_metrics(rank, size, N, p, metrics, verbose);
    
    if (show_results) {
        for (int i = 0; i < size; i++) {
            if (rank == i) {
                print_results(rank, p, original_data, local_data, broadcasted_data,
                            local_ranking, reduced_ranking);
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    
    MPI_Comm_free(&row_comm);
    MPI_Finalize();
    return 0;
}