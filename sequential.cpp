#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <random>
#include <cstdlib>
#include <chrono>

using namespace std;
using namespace chrono;

struct TimeMetrics {
    double total_time;
    double sort_time;
    double ranking_time;
};

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

vector<int> sequential_ranking_sort(const vector<int>& data, TimeMetrics& metrics) {
    int N = data.size();
    
    auto start_sort = high_resolution_clock::now();
    vector<int> sorted_data = data;
    sort(sorted_data.begin(), sorted_data.end());
    auto end_sort = high_resolution_clock::now();
    metrics.sort_time = duration<double>(end_sort - start_sort).count();
    
    auto start_ranking = high_resolution_clock::now();
    vector<int> rankings(N);
    for (int i = 0; i < N; i++) {
        int count = upper_bound(sorted_data.begin(), sorted_data.end(), data[i]) 
                    - sorted_data.begin();
        rankings[i] = count;
    }
    auto end_ranking = high_resolution_clock::now();
    metrics.ranking_time = duration<double>(end_ranking - start_ranking).count();
    
    return rankings;
}

long long calculate_flops(int N) {
    long long sort_ops = N * log2(N);
    long long ranking_ops = N * log2(N);
    return sort_ops + ranking_ops;
}

void print_metrics(int N, const TimeMetrics& metrics, bool verbose) {
    cout << "\n" << string(80, '=') << "\n";
    cout << "RANKING SORT SECUENCIAL - MÉTRICAS DE RENDIMIENTO\n";
    cout << string(80, '=') << "\n";
    
    cout << fixed << setprecision(6);
    cout << "N (elementos):           " << N << "\n\n";
    
    cout << "TIEMPOS (segundos):\n";
    cout << "  Total:                 " << metrics.total_time << "\n";
    
    if (verbose) {
        cout << "  Sort:                  " << metrics.sort_time 
             << " (" << (metrics.sort_time/metrics.total_time*100) << "%)\n";
        cout << "  Ranking:               " << metrics.ranking_time 
             << " (" << (metrics.ranking_time/metrics.total_time*100) << "%)\n";
    }
    
    long long flops = calculate_flops(N);
    double flops_per_sec = flops / metrics.total_time;
    double mflops = flops_per_sec / 1e6;
    double gflops = flops_per_sec / 1e9;
    
    cout << "\nFLOPs:\n";
    cout << "  Operaciones estimadas: " << flops << "\n";
    cout << "  Velocidad (FLOP/s):    " << flops_per_sec << "\n";
    cout << "  Velocidad (MFLOP/s):   " << mflops << "\n";
    cout << "  Velocidad (GFLOP/s):   " << gflops << "\n";
    
    double throughput = N / metrics.total_time;
    cout << "\nTHROUGHPUT:\n";
    cout << "  Elementos/segundo:     " << throughput << "\n";
    
    cout << string(80, '=') << "\n";
    
    cout << "\nFORMATO CSV (para análisis):\n";
    cout << "P,N,total_time,flops,flops_per_sec,mflops,gflops,throughput\n";
    cout << "1," << N << "," 
         << metrics.total_time << "," 
         << flops << ","
         << flops_per_sec << ","
         << mflops << ","
         << gflops << ","
         << throughput << "\n";
}

void print_sample_results(const vector<int>& original, const vector<int>& rankings, int n_samples = 20) {
    cout << "\nMUESTRA DE RESULTADOS (primeros " << n_samples << " elementos):\n";
    cout << "Índice | Original | Ranking\n";
    cout << string(30, '-') << "\n";
    
    for (int i = 0; i < min(n_samples, (int)original.size()); i++) {
        cout << setw(6) << i << " | " 
             << setw(8) << original[i] << " | " 
             << setw(7) << rankings[i] << "\n";
    }
}

int main(int argc, char** argv) {
    bool verbose = false;
    bool show_results = false;
    
    if (argc < 4) {
        cerr << "Uso: " << argv[0] << " <N> <min> <max> [opciones]\n";
        cerr << "  N:   Número de elementos a ordenar\n";
        cerr << "  min: Valor mínimo para números aleatorios\n";
        cerr << "  max: Valor máximo para números aleatorios\n";
        cerr << "\nOpciones:\n";
        cerr << "  -v, --verbose     Mostrar desglose detallado de tiempos\n";
        cerr << "  -r, --results     Mostrar muestra de resultados\n";
        cerr << "\nEjemplos:\n";
        cerr << "  " << argv[0] << " 1000 1 100\n";
        cerr << "  " << argv[0] << " 1000 1 100 -v\n";
        cerr << "  " << argv[0] << " 1000 1 100 -v -r\n";
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
        cerr << "ERROR: N debe ser un número positivo\n";
        return 1;
    }
    
    if (min_val >= max_val) {
        cerr << "ERROR: min debe ser menor que max\n";
        return 1;
    }
    
    vector<int> data = generate_random_array(N, min_val, max_val);
    
    cout << "\nRANKING SORT SECUENCIAL\n";
    cout << "N = " << N << " elementos\n";
    cout << "Rango: [" << min_val << ", " << max_val << "]\n";
    
    TimeMetrics metrics = {0};
    
    auto total_start = high_resolution_clock::now();
    vector<int> rankings = sequential_ranking_sort(data, metrics);
    auto total_end = high_resolution_clock::now();
    
    metrics.total_time = duration<double>(total_end - total_start).count();
    
    print_metrics(N, metrics, verbose);
    
    if (show_results) {
        print_sample_results(data, rankings);
    }
    
    return 0;
}