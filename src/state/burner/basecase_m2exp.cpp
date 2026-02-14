#include "flint/fmpz.h"
#include "flint/ulong_extras.h"
#include "kernels.h"
#include <stdexcept>

Basecase_m2exp::Basecase_m2exp(Context* ctx) : Basecase_simple(ctx) {
    mlog = n_flog(ctx->task.collatz.m, 2);
    if (ctx->task.collatz.m != (1<<mlog)) {
        throw std::runtime_error("Failed to construct basecase m2exp context because m was not a power of 2");
    }
}

void Basecase_m2exp::pushL(timed_fmpz* x_export) {
    fmpz* stored = &storage.value;
    fmpz_fdiv_q_2exp(&x_export->value, stored, mlog*(1<<power));
    x_export->iterations = storage.iterations;
    fmpz_fdiv_r_2exp(stored, stored, mlog*(1<<power));
}

// Takes import and writes to export.
void Basecase_m2exp::step(timed_fmpz* x_import) {
    fmpz* stored = &storage.value;
    ASSERT_SYNCED(storage, *x_import);
    fmpz_add(stored, stored, &x_import->value);

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
        fmpz_fdiv_q_2exp(stored, stored, mlog);
    }
    storage.iterations += n;
}




