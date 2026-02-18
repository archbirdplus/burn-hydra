#include <flint/ulong_extras.h>
#include <iostream>
#include <stdexcept>

#include "kernels.h"

Burner_m2exp::Burner_m2exp(Context* global_ctx, std::unique_ptr<Burner_MPI> upper_ctx, std::unique_ptr<Basecase_simple> basecase_ctx) : Burner_singlethreaded(global_ctx, std::move(upper_ctx), std::move(basecase_ctx)) {
    mlog = n_flog(global_ctx->task->collatz.m, 2);
    if (global_ctx->task->collatz.m != (1<<mlog)) {
        throw std::runtime_error("Failed to construct burner m2exp context because m was not a power of 2");
    }
}

Burner_m2exp::~Burner_m2exp() {
    
}

void Burner_m2exp::tick(uint64_t n) {
    Workspace* ws = global_context->workspace.get();

    uint64_t scale = scale_next[n];
    uint64_t p2 = mlog*((uint64_t)1<<scale);
    fmpz_mul(&storage[n].value, &storage[n].value, &(ws->pR[scale]));
    fmpz_fdiv_r_2exp(&undercarry[n].value, &storage[n].value, p2);
    fmpz_fdiv_q_2exp(&storage[n].value, &storage[n].value, p2);
    storage[n].iterations += (uint64_t) 1 << scale;
    undercarry[n].iterations = storage[n].iterations;
    // set undercarry[n]
}

void Burner_m2exp::pushL(uint64_t n) {
    if ((uint64_t) n == length-1 && !upper_context->can_push_left) return;

    uint64_t scale = scale_self[n];
    uint64_t p2 = mlog*((uint64_t)1<<scale);
    fmpz_fdiv_q_2exp(&overcarry[n+1].value, &storage[n].value, p2);
    fmpz_fdiv_r_2exp(&storage[n].value, &storage[n].value, p2);
    overcarry[n+1].iterations = storage[n].iterations;
    // set overcarry[n+1]
    if ((uint64_t) n == length-1) {
        upper_context->pushL(&overcarry[length]);
    }
}

void Burner_m2exp::recurse(int64_t n) {
    if (n < 0) return;
    uint64_t pow = 1 << scale_delta[n];
    for (uint32_t i = 0; i < pow; i++) {
        exchange(n); // exchange can have multiple orders inside itself
        tick(n); // tick and recurse can happen in parallel
        recurse(n-1);
    }
}

uint64_t Burner_m2exp::step() {
    exchange(length);
    recurse(length-1);
    return 1 << scale_self[length-1];
}




