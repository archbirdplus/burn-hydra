#include "kernels.h"
#include <iostream>

Basecase_simple::Basecase_simple(Context* ctx) {
    global_ctx = ctx;
    power = ctx->task->block_sizes.front().front();
    storage = timed_fmpz();
    fmpz_set_ui(&storage.value, ctx->task->initial);
    storage.iterations = 0;
    user_object = 0; // TODO: what should this be initially?
}

Basecase_simple::~Basecase_simple() {
    storage.clear();
}

void Basecase_simple::pushL(timed_fmpz* x_export) {
    fmpz* stored = &storage.value;
    fmpz_fdiv_qr(&x_export->value, stored, stored, &(global_ctx->workspace->pM[power]));
    x_export->iterations = storage.iterations;
}

// Takes import and writes to export.
void Basecase_simple::step(timed_fmpz* x_import) {
    fmpz* stored = &storage.value;
    ASSERT_SYNCED(storage, *x_import);
    fmpz_add(stored, stored, &x_import->value);

    uint64_t n = (uint64_t) 1 << power;
    uint64_t r = global_ctx->task->collatz.r;
    uint64_t m = global_ctx->task->collatz.m;

    scan_fn_t scan_fn = nullptr;
    void* scan_context = nullptr;
    if (global_ctx->task->scan_config) {
        auto config = *(global_ctx->task->scan_config);
        scan_fn = config.scan_fn;
        scan_context = config.scan_context;
    }
    for (uint64_t i = 0; i < n; i++) {
        uint64_t residue = fmpz_fdiv_ui(stored, m);
        if (scan_fn) {
            user_object = scan_fn(scan_context, user_object, residue);
        }
        fmpz_mul_ui(stored, stored, r);
        fmpz_fdiv_q_ui(stored, stored, m);
        // TODO: table steps
        // TODO: user scan memoization
        // TODO: 2exp optimizations
    }
    storage.iterations += n;
}


