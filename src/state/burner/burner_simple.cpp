#include "kernels.h"
#include <iostream>
#include <iomanip>
#include <cassert>

Burner_simple::Burner_simple(Context* global_ctx, Wrapper* wrapper, std::unique_ptr<Basecase_simple> basecase_ctx) {
    global_context = global_ctx;
    basecase_context = std::move(basecase_ctx);
    upper_context = wrapper;
    subscription = wrapper->add_subscriber(this);
    scale_self = subscription.scales;
    this->length = scale_self.size();
    scale_next = {subscription.next_scale};
    scale_next.insert(scale_next.end(), scale_self.begin(), scale_self.end());
    scale_delta = {};
    for (uint64_t i = 0; i < length; i++)
        scale_delta.push_back(scale_self[i] - scale_next[i]);
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

Burner_simple::~Burner_simple() {
    
}

void Burner_simple::tick(int64_t n) {
    if (n == -1) {
        basecase_context->tick();
        return;
    }
    Workspace* ws = global_context->workspace.get();

    uint64_t scale = scale_next[n];
    fmpz_mul(&storage[n].value, &storage[n].value, &(ws->pR[scale]));
    fmpz_fdiv_qr(&storage[n].value, &undercarry[n].value, &storage[n].value, &(ws->pM[scale]));
    storage[n].iterations += (uint64_t) 1 << scale;
    undercarry[n].iterations = storage[n].iterations;
    // set undercarry[n]
}

void Burner_simple::pushR(int64_t n) {
    // assume tick previously happened, setting undercarry[n]
    if (n == 0) {
        if (subscription.can_push_right)
            upper_context->pushR(subscription.id, &undercarry[0]);
        else {}
    }
}

void Burner_simple::pullR(int64_t n) {
    if (n == 0) {
        if (subscription.can_push_right)
            upper_context->pullR(subscription.id, &overcarry[0]);
        else {}
    }
    // assume corresponding pushL previously happened, setting overcarry[n]
    ASSERT_SYNCED(storage[n], overcarry[n]);
    fmpz_add(&storage[n].value, &storage[n].value, &overcarry[n].value);
}

// Exchanges between nodes n <--> n-1
// At n=0, 0 <--> -1 indicates syncing right with the outer burner.
// At n=length, length <--> length-1 indicates syncing left with the outer burner.
void Burner_simple::exchange(int64_t n) {
    // By convention, pull to the left direction before pushing to the right direction.
    // This needs to be synchronized.
    // These calls automatically handle calling to upper context.
    assert(n >= -1);
    pushL(n-1);
    if (n < (int64_t)length) pullR(n);
    if (n < (int64_t)length) pushR(n);
    pullL(n-1);
}

void Burner_simple::pushL(int64_t n) {
    if (n == -1) {
        if (!subscription.can_push_right)
            basecase_context->pushL(&overcarry[0]);
        return;
    }
    if ((uint64_t) n == length-1 && !subscription.can_push_left) return;

    Workspace* ws = global_context->workspace.get();

    fmpz_fdiv_qr(&overcarry[n+1].value, &storage[n].value, &storage[n].value, &(ws->pM[scale_self[n]]));
    overcarry[n+1].iterations = storage[n].iterations;
    // set overcarry[n+1]
    if ((uint64_t) n == length-1) {
        upper_context->pushL(subscription.id, &overcarry[length]);
    }
}

void Burner_simple::pullL(int64_t n) {
    if (n == -1) {
        if (!subscription.can_push_right)
            basecase_context->pullL(&undercarry[0]);
        return;
    }
    if ((uint64_t) n == length-1) {
        if (!subscription.can_push_left) return;
        upper_context->pullL(subscription.id, &undercarry[length]);
    }
    // assume it was otherwise inserted into undercarry[n+1]
    ASSERT_SYNCED(storage[n], undercarry[n+1]);
    fmpz_add(&storage[n].value, &storage[n].value,&undercarry[n+1].value);
}

void Burner_simple::recurse(int64_t n) {
    if (n < 0) {
        tick(n);
        return;
    };
    uint64_t pow = 1 << scale_delta[n];
    for (uint32_t i = 0; i < pow; i++) {
        exchange(n);
        tick(n);
        recurse(n-1);
    }
}

uint64_t Burner_simple::step() {
    exchange(length);
    recurse(length-1);
    return 1 << scale_self[length-1];
}

void Burner_simple::run_until(uint64_t end) {
    uint64_t start = 0;
    while (start < end) {
        start += step();
    }
}



