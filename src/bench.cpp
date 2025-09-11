#include <gmp.h>
#include <vector>
#include <iostream>

#include "common.h"
#include "segment.h"
#include "segment.h"

int main() {
    mpz_t stored; mpz_init(stored);
    mpz_t tmp; mpz_init(tmp);
    vars_t vars = {
        .stored = {stored},
        .tmp = {tmp},
        .block_size = {8},
    };
    data_t data;
    data.vars = &vars;

    mpz_t add; mpz_init(add);
    mpz_t out; mpz_init(out);

    uint64_t e = 8;

    std::vector<double> times = {};
    int max_attempts = 3;
    for (int attempts = 0; attempts < max_attempts; attempts++) {
        mpz_set_ui(add, 3+attempts);
        mpz_set_ui(out, 0);
        mpz_set_ui(stored, 0);
        const start_time_t start = nanos();
        for (int iter = 0; iter < (1<<20); iter++) {
            basecase_burn(&data, out, add, e, 0);
            mpz_mul_ui(out, out, 7); // scramble it a little
            mpz_fdiv_q_2exp(add, out, e); // just truncate it to pass back
        }
        const double time = seconds(nanos()-start);
        std::cout << "  basecase 2^20 took " << time << " s." << std::endl;
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

