#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <random>
#include <chrono>
#include <cstdlib>

using namespace std;
using namespace chrono;

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

vector<int> sequential_ranking_sort(const vector<int>& data) {
    int N = data.size();
    
    vector<int> sorted_data = data;
    sort(sorted_data.begin(), sorted_data.end());
    
    vector<int> rankings(N);
    for (int i = 0; i < N; i++) {
        int count = upper_bound(sorted_data.begin(), sorted_data.end(), data[i]) 
                    - sorted_data.begin();
        rankings[i] = count;
    }
    
    return rankings;
}

long long calculate_flops(int N) {
    long long sort_ops = N * log2(N);
    long long ranking_ops = N * log2(N);
    return sort_ops + ranking_ops;
}

void print_full_metrics(int N, double total_time) {
    cout << "\n" << string(70, '=') << "\n";
    cout << "RANKING SORT SECUENCIAL - MÉTRICAS\n";
    cout << string(70, '=') << "\n";
    cout << fixed << setprecision(6);
    cout << "N:                 " << N << " elementos\n";
    cout << "Tiempo:            " << (total_time * 1000) << " ms\n";
    
    long long flops = calculate_flops(N);
    double flops_per_sec = flops / total_time;
    double gflops = flops_per_sec / 1e9;
    
    cout << "FLOPs estimados:   " << flops << "\n";
    cout << "GFLOP/s:           " << gflops << "\n";
    cout << "Throughput:        " << (N / total_time) << " elem/s\n";
    cout << string(70, '=') << "\n";
}

int main(int argc, char** argv) {
    if (argc < 4) {
        cerr << "Uso: " << argv[0] << " <N> <min> <max> [--time-only]\n";
        cerr << "\nOpciones:\n";
        cerr << "  --time-only    Solo imprime el tiempo (para usar con MPI)\n";
        cerr << "\nEjemplos:\n";
        cerr << "  " << argv[0] << " 1000 1 100\n";
        cerr << "  " << argv[0] << " 1000 1 100 --time-only\n";
        return 1;
    }
    
    int N = atoi(argv[1]);
    int min_val = atoi(argv[2]);
    int max_val = atoi(argv[3]);
    
    bool time_only = false;
    for (int i = 4; i < argc; i++) {
        if (string(argv[i]) == "--time-only") time_only = true;
    }
    
    if (N <= 0) {
        cerr << "ERROR: N debe ser positivo\n";
        return 1;
    }
    
    if (min_val >= max_val) {
        cerr << "ERROR: min < max requerido\n";
        return 1;
    }
    
    vector<int> data = generate_random_array(N, min_val, max_val);
    
    auto start = high_resolution_clock::now();
    vector<int> rankings = sequential_ranking_sort(data);
    auto end = high_resolution_clock::now();
    
    double total_time = duration<double>(end - start).count();
    
    if (time_only) {
        cout << fixed << setprecision(6) << total_time << endl;
    } else {
        print_full_metrics(N, total_time);
    }
    
    return 0;
}