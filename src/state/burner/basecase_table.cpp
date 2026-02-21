#include "flint/fmpz.h"
#include "flint/ulong_extras.h"
#include "kernels.h"
#include <stdexcept>
#include <iostream>

Basecase_table::Basecase_table(Context* ctx) : Basecase_simple(ctx) {
    auto scan_config = global_ctx->task->scan_config;
    if (scan_config && scan_config->scan_block_size != ctx->task->table_size) {
        throw std::runtime_error("Cannot create table bascase: table size not aligned with scan block size");
    }
    uint64_t n = (uint64_t) 1 << power;
    uint64_t step_size = global_ctx->task->table_size;
    if (n % step_size != 0) {
        throw std::runtime_error("Cannot use this step size: it does not divide the basecase size");
    }
}

void Basecase_table::tick() {
    fmpz* stored = &storage.value;

    uint64_t n = (uint64_t) 1 << power;
    uint64_t step_size = global_ctx->task->table_size;
    uint64_t rS = n_pow(global_ctx->task->collatz.r, step_size);
    uint64_t mS = n_pow(global_ctx->task->collatz.m, step_size);

    scan_fn_t scan_fn = nullptr;
    void* scan_context = nullptr;
    if (global_ctx->task->scan_config) {
        auto config = *(global_ctx->task->scan_config);
        scan_fn = config.scan_fn;
        scan_context = config.scan_context;
    }
    for (uint64_t i = 0; i < n; i += step_size) {
        uint64_t residue = fmpz_fdiv_ui(stored, mS);
        if (scan_fn) {
            user_object = scan_fn(scan_context, user_object, residue);
        }
        fmpz_fdiv_q_ui(stored, stored, mS);
        fmpz_mul_ui(stored, stored, rS);
        uint64_t update = ((uint64_t*)global_ctx->workspace->basecase_table)[residue];
        fmpz_add_ui(stored, stored, update);
        // TODO: table steps
        // TODO: user scan memoization
        // TODO: 2exp optimizations
    }
    storage.iterations += n;
}



