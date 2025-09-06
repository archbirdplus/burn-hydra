#include <unistd.h>
#include <mpi.h>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cassert>

#include "common.h"

#include "segment.h"
#include "metrics.h"

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    std::cout << "Rank " << world_rank << " of " << world_size << " processes. Pid " << getpid() << "." << std::endl;

    segment_t segment = {
        world_size = world_size,
        world_rank = world_rank,
    };

    int64_t max_iterations = (uint64_t)1<<28;
    problem_t problem = {
        .initial = 3,
        .iterations = max_iterations,
    };

    config_t config = {
        .block_params= { },
        .prune_bits = false,
    };

    data_t* data = segment_init(&problem, &config, &segment);

    int64_t checkpoint_interval = 1<<28;

    int64_t next_special = 1<<20; // TODO: must be greater than the max size, or should have sub-steps
    int64_t next_checkpoint = 1<<28; // TODO: resolve dynamically
    int64_t iterations = 0;
    while (iterations < max_iterations) {
        if (iterations >= next_checkpoint) {
            assert(iterations == next_checkpoint);
            // TODO: do checkpoint
            next_checkpoint += checkpoint_interval;
        }
        if (iterations >= next_special) {
            assert(iterations == next_special);
            if (segment.world_rank == 0) {
                // TODO: perform fancy logs like H^2^{28}(3) ≡ { } (mod 2^128) ≡ { } (mod 3^128)
                std::cout << "rank " << segment.world_rank << ": " << iterations << " iterations left" << std::endl;
            }
            next_special *= 2;
        }
        int64_t performed = segment_burn(data, std::min(next_special, next_checkpoint) - iterations);
        iterations += performed;
    }
    segment_finalize(data);

    // print_segment_blocks(data);
    print_smallest_mod(data, (uint64_t)1<<32);
    print_smallest_mod(data, 256);

    dump_metrics(data->metrics);

    std::cout << "Done." << std::endl;

    MPI_Finalize();
}

