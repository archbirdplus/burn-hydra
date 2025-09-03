#include <gmp.h>
#include <gmpxx.h>
#include <vector>

#include "segment.h"

void print_segment_blocks(data_t* data) {
    std::vector<mpz_ptr> stored = data->vars->stored;
    int i = 0;
    for (i = 0; i < stored.size(); i++) {
        gmp_printf("segment %d, block %d: %Zd\n", data->segment->world_rank, i, stored[i]);
    }
}

