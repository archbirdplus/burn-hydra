#include <mpi.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <iostream>
#include <cassert>
#include <vector>

#include "common.h"
#include "communicate.h"
#include "segment.h"
#include "metrics.h"
#include "friendly_assert.h"
#include "latencies.h"

int get_opponent(int rank, int size, int step) {
    const bool even = size % 2 == 0;
    int base = even ? size-1 : size;

    if (step > size-1 || (even && step == size-1)) {
        return -1;
    }

    // Berger scheduler for Round-Robin tournaments.
    // A wheel is constructed with an odd number of spokes (players).
    // Each step the wheel is rotated, and each spoke plays
    // against the other spoke on that level.
    // In an even game, the spoke that is alone plays the center.

    if (rank == base) {
        // Center player
        return ((-step) % base + base) % base;
    } else {
        int self_location = (rank + step) % base;
        int other_location = (base - rank - step) % base;
        if (other_location == self_location) {
            // alone, or against center
            if (even) { return base; }
            else { return rank; }
        }
        int other_rank = ((-rank - 2*step) % base + base) % base;
        return other_rank;
    }
}

void test_get_opponent() {
    bool has_error = false;
    bool* e = &has_error;
    friendly_concern_equal(e, 3, get_opponent(0, 4, 0), "rank 0 size 4 step 0");
    friendly_concern_equal(e, 1, get_opponent(0, 4, 1), "rank 0 size 4 step 1");
    friendly_concern_equal(e, 2, get_opponent(0, 4, 2), "rank 0 size 4 step 2");
    friendly_concern_equal(e,-1, get_opponent(0, 4, 3), "rank 0 size 4 step 3");

    friendly_concern_equal(e, 0, get_opponent(3, 4, 0), "rank 3 size 4 step 0");
    friendly_concern_equal(e, 2, get_opponent(3, 4, 1), "rank 3 size 4 step 1");
    friendly_concern_equal(e, 1, get_opponent(3, 4, 2), "rank 3 size 4 step 2");
    friendly_concern_equal(e,-1, get_opponent(3, 4, 3), "rank 3 size 4 step 3");

    friendly_concern_equal(e, 2, get_opponent(1, 4, 0), "rank 1 size 4 step 0");
    friendly_concern_equal(e, 0, get_opponent(1, 4, 1), "rank 1 size 4 step 1");
    friendly_concern_equal(e, 3, get_opponent(1, 4, 2), "rank 1 size 4 step 2");
    friendly_concern_equal(e,-1, get_opponent(1, 4, 3), "rank 1 size 4 step 3");

    friendly_concern_equal(e, 0, get_opponent(0, 5, 0), "rank 0 size 5 step 0");
    friendly_concern_equal(e, 3, get_opponent(0, 5, 1), "rank 0 size 5 step 1");
    friendly_concern_equal(e, 1, get_opponent(0, 5, 2), "rank 0 size 5 step 2");
    friendly_concern_equal(e, 4, get_opponent(0, 5, 3), "rank 0 size 5 step 3");
    friendly_concern_equal(e, 2, get_opponent(0, 5, 4), "rank 0 size 5 step 4");
    friendly_concern_equal(e,-1, get_opponent(0, 5, 5), "rank 0 size 5 step 5");

    friendly_concern_equal(e, 3, get_opponent(2, 5, 0), "rank 2 size 5 step 0");
    friendly_concern_equal(e, 1, get_opponent(2, 5, 1), "rank 2 size 5 step 1");
    friendly_concern_equal(e, 4, get_opponent(2, 5, 2), "rank 2 size 5 step 2");
    friendly_concern_equal(e, 2, get_opponent(2, 5, 3), "rank 2 size 5 step 3");
    friendly_concern_equal(e, 0, get_opponent(2, 5, 4), "rank 2 size 5 step 4");
    friendly_concern_equal(e,-1, get_opponent(2, 5, 5), "rank 2 size 5 step 5");
    exit(has_error ? 1 : 0);
}

uint64_t time_swap(metrics_t* metrics, int rank, int other, fmpz_t in_num, fmpz_t out_num) {
    auto start = nanos();
    // lower node sends first
    if (rank == other) {
        fmpz_set(in_num, out_num);
    } else if (rank < other) {
        send(metrics, other, 0, out_num);
        recv(metrics, other, 0, in_num);
    } else {
        recv(metrics, other, 0, in_num);
        send(metrics, other, 0, out_num);
    }
    return seconds(nanos() - start);
}

stats_t make_stat_against(metrics_t* metrics, latencies_config_t* config, int rank, int other, flint_rand_t rand) {
    stats_t stats = {
        .means = {},
        .stddevs = {},
    };
    MPI_Barrier(MPI_COMM_WORLD);
    for(size_t i = 0; i < config->sizes.size(); i++) {
        fmpz_t send_num; fmpz_init(send_num);
        fmpz_t recv_num; fmpz_init(recv_num);
        fmpz_randbits_unsigned(send_num, rand, (uint64_t)1<<config->sizes[i]);
        vec<double> times = {};
        double mean = 0;
        for (size_t j = 0; j < config->counts.size(); j++) {
            const double t = time_swap(metrics, rank, other, recv_num, send_num);
            times.push_back(t);
            mean += t;
        }
        // TODO: assert?
        mean /= static_cast<double>(times.size());
        double stddev = 0;
        for (size_t j = 0; j < times.size(); j++) {
            const double d = times[j]-mean;
            stddev += d*d;
        }
        stddev = sqrt(stddev)/(static_cast<double>(times.size())-1);
        stats.means.push_back(mean);
        stats.stddevs.push_back(stddev);
        fmpz_clear(send_num);
        fmpz_clear(recv_num);
    }
    return stats;
}

vec<stats_t> make_stats(metrics_t* metrics, latencies_config_t* config, int rank, int size) {
    flint_rand_t rand;
    flint_rand_init(rand);
    int other;
    vec<stats_t> list = {};
    for (int n = 0; n < size; n++) {
        list.push_back({.means={},.stddevs={}});
    }
    for (int n = 0; n < size; n++) {
        other = get_opponent(rank, size, n);
        if (other < 0) { break; }
        std::cout << "rank " << rank << " versus " << other << std::endl;
        list[other] = make_stat_against(metrics, config, rank, other, rand);
    }
    // TODO: should get_opponent use a more consistent policy here?
    if (list[rank].means.size() == 0) {
        std::cout << "rank " << rank << " versus " << rank << std::endl;
        list[rank] = make_stat_against(metrics, config, rank, rank, rand);
    }
    flint_rand_clear(rand);
    return list;
}

vec<latency_matrix_t> gather_stats(latencies_config_t* config, int rank, int world_size) {
    metrics_t metrics;
    init_metrics(&metrics, 0);

    vec<stats_t> stats = make_stats(&metrics, config, rank, world_size);
    // What we have here is, for each node, a stats[other_node][means|stddevs][size];
    // It'll be serialized into doubles[world_size * 2 * size_groups];

    size_t size_groups = config->sizes.size();
    size_t doubles_count = world_size * 2 * size_groups;
    double* doubles = (double*) malloc(doubles_count * sizeof(double));

    for (int target = 0; target < world_size; target++) {
        const size_t target_offset = target * 2 * size_groups;
        for (size_t size_ind = 0; size_ind < size_groups; size_ind++) {
            double mean = stats[target].means[size_ind];
            double stddev = stats[target].stddevs[size_ind];
            doubles[target_offset + 0 * size_groups + size_ind] = mean;
            doubles[target_offset + 1 * size_groups + size_ind] = stddev;
        }
    }

    double* matrix_buffer = nullptr;
    const int root = 0;
    if (rank == root) {
        matrix_buffer = (double*) malloc (world_size * doubles_count * sizeof(double));
    }

    std::cout << "about to gather" << std::endl;
    MPI_Gather(doubles, doubles_count, MPI_DOUBLE, matrix_buffer, doubles_count, MPI_DOUBLE, root, MPI_COMM_WORLD);
    std::cout << "gathered" << std::endl;
    free(doubles);

    if (rank != root) { return {}; }

    free(matrix_buffer);

    vec<latency_matrix_t> latency_matrices = {};

    for (size_t size_ind = 0; size_ind < size_groups; size_ind++) {
        const size_t bytes = world_size * world_size * sizeof(double);
        double* means = (double*) malloc(bytes);
        double* stddevs = (double*) malloc(bytes);
        latency_matrix_t mat = {
            .means = means,
            .stddevs = stddevs,
        };
        latency_matrices.push_back(mat);
        for (int source = 0; source < world_size; source++) {
            const size_t buf_source_offset = source * world_size * 2 * size_groups;
            for (int target = 0; target < world_size; target++) {
                const size_t buf_target_offset = target * 2 * size_groups;
                const size_t pair_offset = buf_source_offset + buf_target_offset;
                double mean = matrix_buffer[pair_offset];
                double stddev = matrix_buffer[pair_offset];
                means[source * world_size + target] = mean;
                stddevs[source * world_size + target] = stddev;
            }
        }
    }
    return latency_matrices;
}

void print_stats(latencies_config_t* config, int world_rank, int world_size, vec<latency_matrix_t> stats) {
    if (world_rank != 0) { return; }
    std::cout << "{\"sizes\": [";
    for (size_t size_ind = 0; size_ind < config->sizes.size(); size_ind++) {
        if (size_ind != 0) {
            std::cout << ", ";
        }
        std::cout << config->sizes[size_ind];
    }
    std::cout << "], \"counts\": [";
    for (size_t size_ind = 0; size_ind < config->sizes.size(); size_ind++) {
        if (size_ind != 0) {
            std::cout << ", ";
        }
        std::cout << config->counts[size_ind];
    }
    std::cout << "], \"means\": [";
    for (size_t size_ind = 0; size_ind < config->sizes.size(); size_ind++) {
        if (size_ind != 0) {
            std::cout << ", ";
        }
        std::cout << "[";
        for (int source = 0; source < world_size; source++) {
            if (source != 0) {
                std::cout << ", ";
            }
            std::cout << "[";
            for (int target = 0; target < world_size; target++) {
                if (target != 0) {
                    std::cout << ", ";
                }
                std::cout << stats[size_ind].means[source * world_size + target];
            }
            std::cout << "]";
        }
        std::cout << "]";
    }
    std::cout << "], \"stddevs\": [";
    for (size_t size_ind = 0; size_ind < config->sizes.size(); size_ind++) {
        if (size_ind != 0) {
            std::cout << ", ";
        }
        std::cout << "[";
        for (int source = 0; source < world_size; source++) {
            if (source != 0) {
                std::cout << ", ";
            }
            std::cout << "[";
            for (int target = 0; target < world_size; target++) {
                if (target != 0) {
                    std::cout << ", ";
                }
                std::cout << stats[size_ind].means[source * world_size + target];
            }
            std::cout << "]";
        }
        std::cout << "]";
    }
    std::cout << "]}" << std::endl;
}
