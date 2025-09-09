
#ifndef COMMON_H
#define COMMON_H

// #include <mpi.h>
// #include <gmp.h>
// #include <stdio.h>
// #include <algorithm>
#include <vector>
#include <cstdint>

typedef struct problem {
    uint64_t initial;
    int64_t iterations;
} problem_t;

typedef struct config {
    std::vector<std::vector<uint64_t>> block_sizes_funnel;
    std::vector<std::vector<uint64_t>> block_sizes_chain;
    std::vector<std::vector<uint64_t>> block_sizes_used;
    uint64_t global_block_max; // size of largest block in system
    bool prune_bits;
    int64_t checkpoint_interval;
} config_t;

typedef struct segment {
    int world_size;
    int world_rank;
    bool is_base_segment;
    bool is_top_segment;
} segment_t;

#endif // COMMON_H

