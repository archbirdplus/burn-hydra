#include "kernels.h"

Burner_singlethreaded::Burner_singlethreaded(Context* global_ctx, Burner_MPI* upper_ctx, vec<uint32_t> scales, uint32_t next_scale) {
    global_context = global_ctx;
    upper_context = upper_ctx;
    this->length = scales.size();
    scale_self = scales;
    scale_next = {next_scale};
    scale_next.insert(scale_next.end(), scale_self.begin(), scale_self.end());
    scale_delta = {};
    for (uint64_t i = 0; i < length; i++)
        scale_delta.push_back(scale_self[i] - scale_next[i]);
    storage = {};
    undercarry = {};
    overcarry = {};
    for (uint64_t i = 0; i < length; i++) {
        storage.push_back(timed_fmpz());
        storage.back().iterations = 0;
    }
    for (uint64_t i = 0; i < length+1; i++) {
        undercarry.push_back(timed_fmpz());
        overcarry.push_back(timed_fmpz());
        undercarry.back().iterations = 0;
        overcarry.back().iterations = 0;
    }
}

Burner_singlethreaded::~Burner_singlethreaded() {
    
}

void Burner_singlethreaded::tick(uint64_t n) {
    Workspace* ws = &global_context->workspace;

    fmpz_mul(&storage[n].fmpz, &storage[n].fmpz, &(ws->pR[scale_next[n]]));
    fmpz_fdiv_qr(&storage[n].fmpz, &undercarry[n].fmpz, &storage[n].fmpz, &(ws->pM[scale_next[n]]));
    storage[n].iterations += (uint64_t) 1 << scale_self[n];
    undercarry[n].iterations = storage[n].iterations;
    // set undercarry[n]
}

void Burner_singlethreaded::pushR(uint64_t n) {
    // assume tick previously happened, setting undercarry[n]
    if (n == 0)
        upper_context->pushR(&undercarry[0]);
}

void Burner_singlethreaded::pullR(uint64_t n) {
    if (n == 0)
        upper_context->pullR(&overcarry[0]);
    // assume corresponding pushL previously happened, setting overcarry[n]
    ASSERT_SYNCED(storage[n], overcarry[n]);
    fmpz_add(&storage[n].fmpz, &storage[n].fmpz, &overcarry[n].fmpz);
}

void Burner_singlethreaded::pushL(uint64_t n) {
    if ((uint64_t) n == length-1 && !upper_context->can_push_left) return;

    Workspace* ws = &global_context->workspace;

    fmpz_fdiv_qr(&overcarry[n+1].fmpz, &storage[n].fmpz, &storage[n].fmpz, &(ws->pM[scale_self[n]]));
    overcarry[n+1].iterations = storage[n].iterations;
    // set overcarry[n+1]
    if ((uint64_t) n == length-1) {
        upper_context->pushL(&overcarry[length]);
    }
}

void Burner_singlethreaded::pullL(uint64_t n) {
    if ((uint64_t) n == length-1) {
        upper_context->pullL(&undercarry[length]);
    }
    // assume it was otherwise inserted into undercarry[n+1]
    ASSERT_SYNCED(storage[n], undercarry[n+1]);
    fmpz_add(&storage[n].fmpz, &storage[n].fmpz,&undercarry[n+1].fmpz);
}

void Burner_singlethreaded::recurse(int64_t n) {
    if (n < 0) return;
    uint64_t pow = 1 << scale_delta[n];
    for (uint32_t i = 0; i < pow; i++) {
        tick(n);
        pushR(n);
        recurse(n-1);
        pullR(n);
    }
    pushL(n);
    pullL(n);
}

uint64_t Burner_singlethreaded::step() {
    recurse(length-1);
    return 1 << scale_self[length-1];
}



