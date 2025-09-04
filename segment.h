#ifndef SEGMENT_H
#define SEGMENT_H

#include <gmp.h>
#include <gmpxx.h>
#include <vector>

#include "common.h"

typedef struct vars {
    mpz_ptr update;
    std::vector<mpz_ptr> p3;
    std::vector<mpz_ptr> tmp;
    std::vector<mpz_ptr> stored;
    std::vector<uint64_t> block_size; // from left to right, including input (stored) and output (not stored) sizes
} vars_t;

typedef struct data {
    problem_t* problem;
    config_t* config;
    segment_t* segment;
    vars_t* vars;
} data_t;

data_t* segment_init(problem_t*, config_t*, segment_t*);
int segment_burn(data_t*, int64_t);
void segment_finalize(data_t*);
void print_segment_blocks(data_t*);
void print_smallest_mod(data_t* data, uint64_t mod);

#endif // SEGMENT_H

