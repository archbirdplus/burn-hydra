#include <gmp.h>
#include <gmpxx.h>
#include <iostream>
#include <vector>

#include "segment.h"
#include "communicate.h"

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

// Requires all segments to be on the same iteration.
// TODO: assert the above
// Requires all segments to use the same base.
// Gathers residues mod base^exp from all nodes onto rank 0 to print.
void print_signature(data_t* data, uint64_t base, uint64_t exp) {
    // timer
    const std::vector<mpz_ptr> stored = data->vars->stored;
    const std::vector<uint64_t> global_offset = data->vars->global_offset;

    const int root = 0;
    mpz_t res; mpz_init(res); mpz_set_ui(res, 0);
    mpz_t tmp; mpz_init(tmp);
    mpz_t mod; mpz_init(mod);
    mpz_t scale; mpz_init(scale);
    mpz_t two; mpz_init(two); mpz_set_ui(two, 2);

    mpz_ui_pow_ui(mod, base, exp);

    const int count = stored.size();
    for (int i = 0; i < count; i++) {
        mpz_powm_ui(scale, two, global_offset[i], mod);
        mpz_mod(tmp, stored[i], mod);
        mpz_mul(tmp, tmp, scale);
        mpz_add(res, res, tmp);
    }
    mpz_mod(res, res, mod);

    // Unfortunately mpz's do not like being allocated contiguously
    // (are they being reallocated?). Keeping a pointer to mpz pointers
    // prevents a heap corruption in practice.
    mpz_ptr* buffer = nullptr;
    if (data->segment->world_rank == root) {
        // prepare buffer
        buffer = (mpz_ptr*) malloc(data->segment->world_size * sizeof(mpz_ptr));
        for (int i = 0; i < data->segment->world_size; i++) {
            buffer[i] = (mpz_ptr) malloc (sizeof(mpz_t));
            mpz_init(buffer[i]);
        }
    }

    gather(data, res, buffer, root);

    if (data->segment->world_rank == root) {
        // sum again
        mpz_set_ui(res, 0);
        for (int i = 0; i < data->segment->world_size; i++) {
            mpz_add(res, res, buffer[i]);
            mpz_clear(buffer[i]);
        }
        free(buffer);
        // fancy print :D
        mpz_mod(res, res, mod);
        gmp_printf("≡ %Zd (mod %u^%u)", res, base, exp);
    }

    mpz_clear(mod);
    mpz_clear(tmp);
    mpz_clear(res);
    // timer
}

void print_special_2exp(data_t* data, int64_t e) {
    const segment_t* segment = data->segment;
    if (segment->world_rank == 0) {
        gmp_printf("H^2^%d(%u) ", e, data->problem->initial);
    }
    // Note that this function must be called by every segment.
    print_signature(data, 2, 128);
    if (segment->world_rank == 0) {
        gmp_printf(" ");
    }
    print_signature(data, 3, 128);
    if (segment->world_rank == 0) {
        gmp_printf("\n");
    }
}

