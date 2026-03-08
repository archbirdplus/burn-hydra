#pragma once

#include "types.h"

typedef struct layout {
    vecvec<uint64_t> block_sizes_ramp;
    vecvec<uint64_t> block_sizes_plat;
    vecvec<uint64_t> thread_breaks;
} layout_t;

typedef struct parse_results {
    layout_t layout;
    bool prune;
    uint64_t iterations;
    uint64_t checkpoint_interval;
    int64_t start_value;
} parse_results_t;

void parse_layout(layout_t*, const char*);
//void parse_args(problem_t* problem, config_t* config, int argc, char** argv);


