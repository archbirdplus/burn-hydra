#include "kernels.h"
#include <iostream>
#include <iomanip>

Burner_openmp::Burner_openmp(Context* global_ctx, std::unique_ptr<Burner_MPI> upper_ctx, std::unique_ptr<Basecase_simple> basecase_ctx) {
    global_context = global_ctx;
    upper_context = std::move(upper_ctx);
    basecase_context = std::move(basecase_ctx);
    scale_self = upper_context->local_scales;
    this->length = scale_self.size();
    scale_next = {upper_context->local_next_scale};
    scale_next.insert(scale_next.end(), scale_self.begin(), scale_self.end());
    scale_delta = {};
    for (uint64_t i = 0; i < length; i++)
        scale_delta.push_back(scale_self[i] - scale_next[i]);
    storage = {};
    undercarry = {};
    overcarry = {};
    for (uint64_t i = 0; i < length; i++) {
        storage.push_back(locked_fmpz());
    }
    for (uint64_t i = 0; i < length+1; i++) {
        undercarry.push_back(locked_fmpz());
        overcarry.push_back(locked_fmpz());
    }
}

void Burner_openmp::tick(uint64_t n) {
    Workspace* ws = &global_context->workspace;

    uint64_t scale = scale_next[n];
    timed_fmpz* stored = storage[n].lock_unknown();
    fmpz_mul(&stored->fmpz, &stored->fmpz, &(ws->pR[scale]));
    timed_fmpz* carry = undercarry[n].lock_unknown();
    fmpz_fdiv_qr(&stored->fmpz, &carry->fmpz, &stored->fmpz, &(ws->pM[scale]));
    stored->iterations += (uint64_t) 1 << scale;
    carry->iterations = stored->iterations;
    storage[n].unlock();
    undercarry[n].unlock();
}

void Burner_openmp::pushR(uint64_t n) {
    // assume tick previously happened, setting undercarry[n]
    if (n == 0) {
        timed_fmpz* x = undercarry[0].lock_unknown();
        if (upper_context->can_push_right)
            upper_context->pushR(x);
        else
            basecase_context->step(x);
        undercarry[0].unlock();
    }
}

void Burner_openmp::pullR(uint64_t n) {
    if (n == 0) {
        timed_fmpz* x = overcarry[0].lock_unknown();
        if (upper_context->can_push_right)
            upper_context->pullR(x);
        else
            basecase_context->pushL(x);
        overcarry[0].unlock();
    }
    // assume corresponding pushL previously happened, setting overcarry[n]
    timed_fmpz* stored = storage[n].lock_unknown();
    timed_fmpz* carry = overcarry[n].lock(stored->iterations);
    fmpz_add(&stored->fmpz, &stored->fmpz, &carry->fmpz);
    storage[n].unlock();
    overcarry[n].unlock();
}

// Exchanges between nodes n <--> n-1
// At n=0, 0 <--> -1 indicates syncing right with the outer burner.
// At n=length, length <--> length-1 indicates syncing left with the outer burner.
void Burner_openmp::exchange(uint64_t n) {
    // By convention, pull to the left direction before pushing to the right direction.
    // This needs to be synchronized.
    // These calls automatically handle calling to upper context.
    if (n > 0) {
        #pragma omp parallel sections
        {
            #pragma omp section
            {
                if (n > 0) pushL(n-1);
                if (n <= length) pullR(n);
            }
            #pragma omp section
            {
                if (n <= length) pushR(n);
                if (n > 0) pullL(n-1);
            }
        }
    } else {
        if (n < length) pullR(n);
        if (n < length) pushR(n);
    }
}

void Burner_openmp::pushL(uint64_t n) {
    if ((uint64_t) n == length-1 && !upper_context->can_push_left) return;

    Workspace* ws = &global_context->workspace;

    timed_fmpz* stored = storage[n].lock_unknown();
    timed_fmpz* carry = overcarry[n+1].lock_unknown();
    fmpz_fdiv_qr(&carry->fmpz, &stored->fmpz, &stored->fmpz, &(ws->pM[scale_self[n]]));
    carry->iterations = stored->iterations;
    storage[n].unlock();
    // set overcarry[n+1]
    if ((uint64_t) n == length-1) {
        upper_context->pushL(carry);
    }
    overcarry[n+1].unlock();
}

void Burner_openmp::pullL(uint64_t n) {
    if ((uint64_t) n == length-1 && !upper_context->can_push_left) return;
    timed_fmpz* carry = undercarry[n+1].lock_unknown();
    if ((uint64_t) n == length-1) {
        upper_context->pullL(carry);
    }
    timed_fmpz* stored = storage[n].lock(carry->iterations);
    fmpz_add(&stored->fmpz, &stored->fmpz, &carry->fmpz);
    storage[n].unlock();
    undercarry[n+1].unlock();
}

void Burner_openmp::recurse(int64_t n) {
    if (n < 0) return;
    uint64_t pow = 1 << scale_delta[n];
    for (uint32_t i = 0; i < pow; i++) {
        exchange(n); // exchange can have multiple orders inside itself
        tick(n); // tick and recurse can happen in parallel
        recurse(n-1);
    }
}

uint64_t Burner_openmp::step() {
    exchange(length);
    recurse(length-1);
    return 1 << scale_self[length-1];
}



