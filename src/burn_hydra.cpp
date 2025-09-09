#include <unistd.h>
#include <mpi.h>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cassert>

#include "common.h"

#include "parse.h"
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

    problem_t problem;
    config_t config;

    parse_args(&problem, &config, argc, argv);

    data_t* data = segment_init(&problem, &config, &segment);

    // TODO: maybe allow sub-step based specials?
    int64_t next_special = config.global_block_max;
    int64_t next_checkpoint = config.checkpoint_interval;
    int64_t iterations = 0;
    while (iterations < problem.iterations) {
        if (iterations >= next_checkpoint) {
            assert(iterations == next_checkpoint);
            // TODO: do checkpoint
            std::cout << "TODO: checkpoint" << std::endl;
            next_checkpoint += config.checkpoint_interval;
        }
        if (iterations >= 1<<next_special) {
            assert(iterations == 1<<next_special);
            print_special_2exp(data, next_special);
            next_special += 1;
        }
        int64_t performed = segment_burn(data, std::min((int64_t)1<<next_special, next_checkpoint) - iterations);
        iterations += performed;
    }
    segment_finalize(data);

    if (iterations == 1<<next_special) {
        print_special_2exp(data, next_special);
    } else {
        // not great but whatever, should confuse someone
        print_special_2exp(data, -1);
    }

    dump_metrics(data->metrics);

    std::cout << "Done." << std::endl;

    MPI_Finalize();
}

