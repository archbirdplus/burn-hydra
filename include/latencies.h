#ifndef LATENCIES_H
#define LATENCIES_H

#include <vector>

template<typename T> using vec = std::vector<T>;

typedef struct latencies_config {
    vec<uint64_t> sizes;
    vec<uint64_t> counts;
} latencies_config_t;

// one of these for every pair of nodes
typedef struct latency_stats {
    vec<double> means;
    vec<double> stddevs;
} stats_t;

// one of these for every size of message
typedef struct latency_matrix {
    double* means;
    double* stddevs;
} latency_matrix_t;

void test_get_opponent();
vec<latency_matrix_t> gather_stats(latencies_config_t* config, int rank, int world_size);
void print_stats(latencies_config_t* config, int world_rank, int world_size, vec<latency_matrix_t> stats);

#endif // LATENCIES_H

