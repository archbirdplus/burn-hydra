#include "kernels.h"
#include <iostream>

Burner_basecase::Burner_basecase(Context* ctx) {
    global_ctx = ctx;
    power = ctx->task.block_sizes.front().front();
    storage = timed_fmpz();
    fmpz_set_ui(&storage.fmpz, ctx->task.initial);
    storage.iterations = 0;
    user_object = 0; // TODO: what should this be initially?
}

void Burner_basecase::pushL(timed_fmpz* x_export) {
    fmpz* stored = &storage.fmpz;
    fmpz_fdiv_qr(&x_export->fmpz, stored, stored, &(global_ctx->workspace.pM[power]));
    x_export->iterations = storage.iterations;
}

// Takes import and writes to export.
void Burner_basecase::step(timed_fmpz* x_import) {
    fmpz* stored = &storage.fmpz;
    ASSERT_SYNCED(storage, *x_import);
    fmpz_add(stored, stored, &x_import->fmpz);

    uint64_t n = (uint64_t) 1 << power;
    uint64_t r = global_ctx->task.collatz.r;
    uint64_t m = global_ctx->task.collatz.m;

    scan_fn_t scan_fn = nullptr;
    void* scan_context = nullptr;
    if (global_ctx->task.scan_config) {
        auto config = *(global_ctx->task.scan_config);
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


