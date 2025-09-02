#include <gmp.h>
#include <gmpxx.h>
#include <vector>

#include "segment.h"

void print_segment_blocks(data_t* data) {
    std::vector<mpz_class> stored = data->vars->stored;
    std::vector<mpz_class>::iterator it;
    int i = 0; // stored.size();
    for (i = 0; i < stored.size(); i++) {
        gmp_printf("segment %d, block %d: %Zd\n", data->segment->world_rank, i, stored[i].get_mpz_t());
    }
}

