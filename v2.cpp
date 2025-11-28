#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <random>
#include <cstdlib>

using namespace std;


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

// FUNCIÓN PARA GENERAR NÚMEROS ALEATORIOS
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

// FASE 1: INPUT + GOSSIP COMBINADOS
vector<int> phase1_input_gossip(
    const vector<int>& global_data,
    int rank, int size, int p
) {
    int N = global_data.size();
    int P = size;
    int elements_per_group = N / P;  // N/P
    int elements_per_process = elements_per_group * p;  // N/p
    
    vector<int> local_data;
    local_data.reserve(elements_per_process);
    
    // (grupo_id mod p) == (rank mod p)
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

// FASE 2: BROADCAST HORIZONTAL
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

// FASE 3: SORT LOCAL
void phase3_sort(vector<int>& local_data) {
    sort(local_data.begin(), local_data.end());
}

// FASE 4: LOCAL RANKING
vector<int> phase4_local_ranking(
    const vector<int>& sorted_local, 
    const vector<int>& broadcasted
) {
    vector<int> ranking;
    ranking.reserve(broadcasted.size());
    
    // para cada elemento del arreglo broadcasted
    for (int value : broadcasted) {
        // cuantos elementos en sorted_local son <= value
        int count = upper_bound(sorted_local.begin(), sorted_local.end(), value) 
                    - sorted_local.begin();
        ranking.push_back(count);
    }
    
    return ranking;
}

// FASE 5: REDUCE HORIZONTAL
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
    
    // Arreglo original
    cout << "  Original:     ";
    for (size_t i = 0; i < original.size(); i++) {
        cout << setw(4) << original[i];
        if (i < original.size() - 1) cout << " ";
    }
    cout << "\n";
    
    // Arreglo ordenado
    cout << "  Sorted:       ";
    for (size_t i = 0; i < sorted_local.size(); i++) {
        cout << setw(4) << sorted_local[i];
        if (i < sorted_local.size() - 1) cout << " ";
    }
    cout << "\n";
    
    // Arreglo broadcasted
    cout << "  Broadcasted:  ";
    for (size_t i = 0; i < broadcasted.size(); i++) {
        cout << setw(4) << broadcasted[i];
        if (i < broadcasted.size() - 1) cout << " ";
    }
    cout << "\n";
    
    // Ranking local
    cout << "  Local Rank:   ";
    for (size_t i = 0; i < local_ranking.size(); i++) {
        cout << setw(4) << local_ranking[i];
        if (i < local_ranking.size() - 1) cout << " ";
    }
    cout << "\n";
    
    // Ranking reduce (solo para diagonales)
    if (is_diag) {
        cout << "  Reduced Rank: ";
        for (size_t i = 0; i < reduced_ranking.size(); i++) {
            cout << setw(4) << reduced_ranking[i];
            if (i < reduced_ranking.size() - 1) cout << " ";
        }
        cout << "\n";
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (argc != 4) {
        if (rank == 0) {
            cerr << "Uso: " << argv[0] << " <N> <min> <max>\n";
            cerr << "  N:   Número de elementos a ordenar\n";
            cerr << "  min: Valor mínimo para números aleatorios\n";
            cerr << "  max: Valor máximo para números aleatorios\n";
            cerr << "\nEjemplo: " << argv[0] << " 18 1 100\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    int N = atoi(argv[1]);
    int min_val = atoi(argv[2]);
    int max_val = atoi(argv[3]);
    
    if (N <= 0) {
        if (rank == 0) {
            cerr << "ERROR: N debe ser un número positivo\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    if (min_val >= max_val) {
        if (rank == 0) {
            cerr << "ERROR: min debe ser menor que max\n";
        }
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
        cout << "N = " << N << " elementos\n";
        cout << "P = " << size << " procesos (p = " << p << ")\n";
        cout << "Elementos por proceso: " << N/p << "\n";
        cout << "Rango: [" << min_val << ", " << max_val << "]\n";
        
        cout << "\nArreglo original (primeros 20): ";
        for (int i = 0; i < min(20, N); i++) {
            cout << global_data[i] << " ";
        }
        if (N > 20) cout << "...";
        cout << "\n";
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    auto [row, col] = rank_to_position(rank, p);
    MPI_Comm row_comm;
    MPI_Comm_split(MPI_COMM_WORLD, row, col, &row_comm);
    
    // FASE 1: INPUT + GOSSIP COMBINADOS
    vector<int> local_data = phase1_input_gossip(global_data, rank, size, p);
    vector<int> original_data = local_data;
    
    // FASE 2: BROADCAST HORIZONTAL
    vector<int> broadcasted_data = phase2_broadcast(local_data, rank, size, p, row_comm);
    
    // FASE 3: SORT LOCAL
    phase3_sort(local_data);
    
    // FASE 4: LOCAL RANKING
    vector<int> local_ranking = phase4_local_ranking(local_data, broadcasted_data);
    
    // FASE 5: REDUCE HORIZONTAL
    vector<int> reduced_ranking = phase5_reduce(local_ranking, rank, size, p, row_comm);

    for (int i = 0; i < size; i++) {
        if (rank == i) {
            print_results(rank, p, original_data, local_data, broadcasted_data,
                         local_ranking, reduced_ranking);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    MPI_Comm_free(&row_comm);
    MPI_Finalize();
    return 0;
}