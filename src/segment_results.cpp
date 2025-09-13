#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <iostream>
#include <vector>

#include "segment.h"
#include "communicate.h"

void print_segment_blocks(data_t* data) {
    std::vector<fmpz> stored = data->vars->stored;
    int i;
    for (i = stored.size()-1; i >= 0; i--) {
        flint_printf("segment %d, block %d: %{fmpz}\n", data->segment->world_rank, i, &stored[i]); // TODO: ampersand or not?
    }
}

void print_smallest_mod(data_t* data, uint64_t mod) {
    fmpz_t a; fmpz_init(a);
    uint64_t m = fmpz_mod_ui(a, &data->vars->stored[data->vars->stored.size()-1], mod);
    std::cout << data->segment->world_rank << "'s smallest block mod " << mod << " is " << m << std::endl;
}

// Requires all segments to be on the same iteration.
// TODO: assert the above
// Requires all segments to use the same base.
// Gathers residues mod base^exp from all nodes onto rank 0 to print.
void print_signature(data_t* data, uint64_t base, uint64_t exp) {
    // timer
    const std::vector<fmpz> stored = data->vars->stored;
    const std::vector<uint64_t> global_offset = data->vars->global_offset;

    const int root = 0;
    fmpz_t res; fmpz_init(res); fmpz_set_ui(res, 0);
    fmpz_t tmp; fmpz_init(tmp);
    fmpz_t mod; fmpz_init(mod);
    fmpz_t scale; fmpz_init(scale);
    fmpz_t two; fmpz_init(two); fmpz_set_ui(two, 2);

    fmpz_ui_pow_ui(mod, base, exp);

    const int count = stored.size();
    for (int i = 0; i < count; i++) {
        fmpz_powm_ui(scale, two, global_offset[i], mod);
        fmpz_mod(tmp, &stored[i], mod);
        fmpz_mul(tmp, tmp, scale);
        fmpz_add(res, res, tmp);
    }
    fmpz_mod(res, res, mod);

    // TODO: check that this doesn't corrupt things again
    fmpz* buffer = nullptr;
    if (data->segment->world_rank == root) {
        // prepare buffer
        buffer = (fmpz*) malloc(data->segment->world_size * sizeof(fmpz));
        for (int i = 0; i < data->segment->world_size; i++) {
            // fmpz's default to 0, then turn into pointers as needed
            fmpz_init(&buffer[i]);
            _fmpz_promote(&buffer[i]);
        }
    }

    gather(data, res, buffer, root);

    if (data->segment->world_rank == root) {
        // sum again
        fmpz_set_ui(res, 0);
        for (int i = 0; i < data->segment->world_size; i++) {
            fmpz_add(res, res, &buffer[i]);
            fmpz_clear(&buffer[i]);
        }
        free(buffer);
        // fancy print :D
        fmpz_mod(res, res, mod);
        flint_printf("≡ %{fmpz} (mod %u^%u)", &res, base, exp);
    }

    fmpz_clear(mod);
    fmpz_clear(tmp);
    fmpz_clear(res);
    // timer
}

void print_special_2exp(data_t* data, int64_t e) {
    const segment_t* segment = data->segment;
    if (segment->world_rank == 0) {
        flint_printf("H^2^%d(%u) ", e, data->problem->initial);
    }
    // Note that this function must be called by every segment.
    print_signature(data, 2, 128);
    if (segment->world_rank == 0) {
        flint_printf(" ");
    }
    print_signature(data, 3, 128);
    if (segment->world_rank == 0) {
        flint_printf("\n");
    }
}

