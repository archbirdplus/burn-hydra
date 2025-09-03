
#include <mpi.h>
#include <iostream>
#include <algorithm>
#include <cstdint>

#include "common.h"

#include "segment.h"

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
        .block_params= { },
        .prune_bits = false,
    };

    // uint64_t input_size = config.block_sizes[world_rank];
    // uint64_t output_size = config.block_sizes[std::clamp(world_rank-1, 0, 2)];

    data_t* data = segment_init(&problem, &config, &segment);
    while (iterations > 0) {
        int performed = segment_burn(data, iterations);
        iterations -= performed;
        // TODO: occasionally checkpoint
        std::cout << "iterations left: " << iterations << std::endl;
    }

    // print_segment_blocks(data);
    print_smallest_mod(data, (uint64_t)1<<32);
    print_smallest_mod(data, 256);

    MPI_Finalize();
}

