
#ifndef COMMON_H
#define COMMON_H

// #include <mpi.h>
// #include <gmp.h>
// #include <stdio.h>
// #include <algorithm>
// #include <cstdint>

typedef struct problem {
    uint64_t initial;
    int64_t iterations;
} problem_t;

typedef struct config {
    uint64_t block_sizes[3];
    bool prune_bits;
} config_t;

typedef struct segment {
    int world_size;
    int world_rank;
} segment_t;

#endif // COMMON_H

