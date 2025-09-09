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

    problem_t problem;
    config_t config = {
        .block_sizes_funnel = {},
        .block_sizes_chain = {},
        .block_sizes_used = {},
        .global_block_max = 0,
        .prune_bits = 0,
        .checkpoint_interval = 0,
    };

    parse_args(&problem, &config, argc, argv);

    segment_t segment = {
        .world_size = world_size,
        .world_rank = world_rank,
        .is_base_segment = 0,
        .is_top_segment = 0,
    };

    data_t* data = segment_init(&problem, &config, &segment);

    // TODO: maybe allow specials at sub-steps?
    int64_t next_special = config.global_block_max;
    int64_t next_checkpoint = config.checkpoint_interval;
    int64_t iterations = 0;
    while (iterations < problem.iterations) {
        if (config.checkpoint_interval && iterations >= next_checkpoint) {
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
        int64_t steps_to_special = (1<<next_special) - iterations;
        int64_t steps = steps_to_special;
        if (config.checkpoint_interval) {
            int64_t steps_to_checkpoint = next_checkpoint - iterations;
            if (steps_to_checkpoint < steps) {
                steps = steps_to_checkpoint;
            }
        }
        int64_t performed = segment_burn(data, steps);
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

