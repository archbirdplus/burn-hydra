#pragma once

#include "types.h"

typedef struct layout {
    vecvec<uint64_t> block_sizes_ramp;
    vecvec<uint64_t> block_sizes_plat;
    vecvec<uint64_t> thread_breaks;
} layout_t;

typedef struct parse_results {
    opt<layout_t> layout;
    opt<bool> prune;
    opt<uint64_t> iterations;
    opt<uint64_t> checkpoint_interval;
    opt<int64_t> start_value;
    opt<uint64_t> flint_threads;
} parse_results_t;

void parse_layout(layout_t*, const char*);
void parse_args(parse_results_t*, int, char**);


