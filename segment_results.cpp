#include <gmp.h>
#include <gmpxx.h>
#include <iostream>
#include <vector>

#include "segment.h"

void print_segment_blocks(data_t* data) {
    std::vector<mpz_ptr> stored = data->vars->stored;
    int i;
    for (i = stored.size()-1; i >= 0; i--) {
        gmp_printf("segment %d, block %d: %Zd\n", data->segment->world_rank, i, stored[i]);
    }
}

void print_smallest_mod(data_t* data, uint64_t mod) {
    mpz_t a; mpz_init(a);
    uint64_t m = mpz_mod_ui(a, data->vars->stored.back(), mod);
    std::cout << data->segment->world_rank << "'s smallest block mod " << mod << " is " << m << std::endl;
}

