
#include <mpi.h>
#include <iostream>
#include <algorithm>
#include <cstdint>

#include "common.h"

#include "segment_burn.h"

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    segment_t segment = {
        world_size = world_size,
        world_rank = world_rank,
    };

    int64_t iterations = 1<<20;
    problem_t problem = {
        .initial = 3,
        .iterations = iterations,
    };

    config_t config = {
        .block_params= {
            ((uint64_t)1 << 9),
            ((uint64_t)1 << 36),
            ((uint64_t)1 << 36),
        },
        .prune_bits = false,
    };

    // uint64_t input_size = config.block_sizes[world_rank];
    // uint64_t output_size = config.block_sizes[std::clamp(world_rank-1, 0, 2)];

    void* data = segment_init(&problem, &config, &segment);
    while (iterations > 0) {
        int performed = segment_burn(data, iterations);
        iterations -= performed;
        // TODO: occasionally checkpoint
        std::cout << "iterations left: " << iterations << std::endl;
    }

    MPI_Finalize();
}

