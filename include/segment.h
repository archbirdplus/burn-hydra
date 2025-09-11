#ifndef SEGMENT_H
#define SEGMENT_H

#include <gmp.h>
#include <gmpxx.h>
#include <vector>

#include "common.h"
#include "metrics.h"

typedef struct vars {
    mpz_ptr update;
    std::vector<mpz_ptr> p3;
    std::vector<mpz_ptr> tmp;
    std::vector<mpz_ptr> stored;

    std::vector<uint64_t> block_size; // from left to right, including input (stored) and output (not stored) sizes; log length
    std::vector<uint64_t> global_offset; // number of bits from the basecase
} vars_t;

typedef struct data {
    problem_t* problem;
    config_t* config;
    segment_t* segment;
    vars_t* vars;
    metrics_t* metrics;
} data_t;

data_t* segment_init(problem_t*, config_t*, segment_t*);
int segment_burn(data_t*, int64_t);
void segment_finalize(data_t*);

// internal objects for benchmarking
void basecase_burn(data_t* data, mpz_t rop, mpz_t add, uint64_t e, int i);

void print_segment_blocks(data_t*);
void print_smallest_mod(data_t*, uint64_t);
void print_signature(data_t*, uint64_t, uint64_t);
void print_special_2exp(data_t*, int64_t);

#endif // SEGMENT_H

