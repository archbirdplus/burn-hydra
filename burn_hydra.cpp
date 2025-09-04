#include <unistd.h>
#include <mpi.h>
#include <iostream>
#include <algorithm>
#include <cstdint>

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

    int64_t iterations = (uint64_t)1<<28;
    problem_t problem = {
        .initial = 3,
        .iterations = iterations,
    };

    config_t config = {
        .block_params= { },
        .prune_bits = false,
    };

    data_t* data = segment_init(&problem, &config, &segment);
    int64_t last_print = iterations;
    while (iterations > 0) {
        if (last_print - iterations > (1<<24)) {
            std::cout << "rank " << segment.world_rank << ": " << iterations << " iterations left" << std::endl;
            last_print = iterations;
        }
        int64_t performed = segment_burn(data, iterations);
        iterations -= performed;
        // TODO: occasionally checkpoint
        // print_segment_blocks(data);
    }
    segment_finalize(data);

    // print_segment_blocks(data);
    print_smallest_mod(data, (uint64_t)1<<32);
    print_smallest_mod(data, 256);

    dump_metrics(data->metrics);

    std::cout << "Done." << std::endl;

    MPI_Finalize();
}

