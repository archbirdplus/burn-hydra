#include <gmp.h>
#include <flint/fmpz.h>
#include <vector>
#include <iostream>

#include "common.h"
#include "segment.h"

int main() {
    // 17 seems to be optimal. It has one more addition step than 16,
    // but one fewer multiplication.
    const uint64_t power = 17;
    basecase_table_t* table = (basecase_table_t*) malloc(((uint64_t)1<<power) * sizeof(basecase_table_t));
    vars_t vars = {
        .update = 0,
        .p3 = {},
        .tmp = {0},
        .stored = {0},
        .basecase_table = table,
        .p3base = 0,
        .table_bits = 0,

        .block_size = {8},
        .global_offset = {0},
    };
    data_t data;
    data.vars = &vars;

    fmpz* stored = &vars.stored[0];
    fmpz_init(stored);
    fmpz* tmp = &vars.tmp[0];
    fmpz_init(tmp);

    const start_time_t setup_start = nanos();
    init_table(&vars, power);
    const double setup_length = seconds(nanos()-setup_start);
    // Generally much less than the actual benchmark.
    std::cout << "Spent " << setup_length << " s on setting up basecase tables." << std::endl;

    fmpz_t add; fmpz_init(add);
    fmpz_t out; fmpz_init(out);

    uint64_t e = 8;
    uint64_t p = 20;

    std::vector<double> times = {};
    int max_attempts = 3;
    for (int attempts = 0; attempts < max_attempts; attempts++) {
        fmpz_set_ui(stored, 3+attempts);
        fmpz_set_ui(add, 0);
        fmpz_set_ui(out, 0);
        const start_time_t start = nanos();
        for (int iter = 0; iter < (1<<p); iter++) {
            basecase_burn(&data, out, add, e, 0);
            fmpz_mul_ui(out, out, 7); // scramble it a little
            fmpz_fdiv_q_2exp(add, out, e); // just truncate it to pass back
        }
        const double time = seconds(nanos()-start);
        fmpz_fdiv_q_2exp(stored, stored, 64);
        flint_printf("  basecase 2^%llu iterations of e=%llu took %f s. (signature: %{fmpz})\n", p, e, time, stored);
        times.push_back(time);
    }
    double mean = 0;
    double stddev = 0;
    for (int attempts = 0; attempts < max_attempts; attempts++) {
        mean += times[attempts];
    }
    mean /= double(max_attempts);
    for (int attempts = 0; attempts < max_attempts; attempts++) {
        stddev += (times[attempts] - mean)*(times[attempts] - mean);
    }
    stddev /= double(max_attempts-1);
    std::cout << "Basecase mean: " << mean << " ± " << stddev << " s." << std::endl;
    return 0;
}

