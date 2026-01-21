#include <iostream>

#include "state.h"
#include "kernels.h"

#include <flint/fmpz.h>
#include "communicate.h"

Burner_basecase::Burner_basecase(Context* ctx) {
    global_ctx = ctx;
    power = ctx->task.block_sizes.front().front();
    fmpz_init_set_ui(storage, ctx->task.initial);
    user_object = 0; // TODO: what should this be initially?
}

// Takes import and writes to export.
void Burner_basecase::step(fmpz* x_import, fmpz* x_export) {
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
        uint64_t residue = fmpz_fdiv_ui(storage, m);
        if (scan_fn) {
            user_object = scan_fn(scan_context, user_object, residue);
        }
        fmpz_mul_ui(storage, storage, r);
        fmpz_fdiv_q_ui(storage, storage, m);
        // TODO: table steps
        // TODO: user scan memoization
        // TODO: 2exp optimizations
    }
    fmpz_add(storage, storage, x_import);
    fmpz_fdiv_qr(x_export, storage, storage, &(global_ctx->workspace.pM[power]));
}

Burner_MPI::Burner_MPI(Context* global_ctx) {
    global_context = global_ctx;
    world_size = global_ctx->task.world_size;
    world_rank = global_ctx->task.world_rank;
    can_push_left = world_rank != world_size - 1;
    can_push_right = world_rank != 0;
    vec<uint32_t> scales = {};
    for (uint64_t i = 0; i < global_ctx->task.block_sizes[world_rank].size(); i++) {
        scales.push_back(static_cast<uint32_t>(global_ctx->task.block_sizes[world_rank][i]));
    }
    uint32_t next_scale = world_rank > 0 ?
        global_ctx->task.block_sizes[world_rank-1].back() :
        scales.front(); // TODO: this depends on the power of the basecase
    node_context = std::unique_ptr<Burner_singlethreaded>(new Burner_singlethreaded(global_ctx, this, scales, next_scale));
    basecase_context = std::unique_ptr<Burner_basecase>(new Burner_basecase(global_ctx));
}

Burner_MPI::~Burner_MPI() {
    
}

void Burner_MPI::syncR(fmpz* x_export, fmpz* x_import) {
    if (!can_push_right) {
        basecase_context->step(x_export, x_import);
    } else {
        sendRight(global_context, x_export);
        receiveRight(global_context, x_import);
    }
}

void Burner_MPI::syncL(fmpz* x_export, fmpz* x_import) {
    if (world_rank == world_size - 1) {
        // TODO: this needs a multiplication to happen correctly
        fmpz_swap(x_import, x_export);
    } else {
        receiveLeft(global_context, x_import);
        sendLeft(global_context, x_export);
    }
}

uint64_t Burner_MPI::step() {
    return node_context->step();
}

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
        storage.push_back(0);
        fmpz_init_set_ui(&storage[i], 0);
    }
    for (uint64_t i = 0; i < length+1; i++) {
        undercarry.push_back(0);
        overcarry.push_back(0);
        fmpz_init_set_ui(&undercarry[i], 0);
        fmpz_init_set_ui(&overcarry[i], 0);
    }
}

Burner_singlethreaded::~Burner_singlethreaded() {
    for (uint64_t i = 0; i < length; i++) {
        fmpz_clear(&storage[i]);
    }
    for (uint64_t i = 0; i < length+1; i++) {
        fmpz_clear(&undercarry[i]);
        fmpz_clear(&overcarry[i]);
    }
}

void Burner_singlethreaded::syncR(uint64_t n) {
    Workspace* ws = &global_context->workspace;

    fmpz_mul(&storage[n], &storage[n], &(ws->pR[scale_next[n]]));
    fmpz_fdiv_qr(&storage[n], &undercarry[n], &storage[n], &(ws->pM[scale_next[n]]));
    // fmpz_fdiv_q_2exp(&undercarry[n], &storage[n], n);
    // fmpz_fdiv_r_2exp(&storage[n], &storage[n], n);
    // TODO: shortcut to basecase if at global minimum
    if (n == 0)
        upper_context->syncR(&undercarry[0], &overcarry[0]);
    fmpz_add(&storage[n], &storage[n], &overcarry[n]);
}

void Burner_singlethreaded::syncL(uint64_t n) {
    if ((uint64_t) n == length-1 && !upper_context->can_push_left) return;

    Workspace* ws = &global_context->workspace;

    fmpz_fdiv_qr(&overcarry[n+1], &storage[n], &storage[n], &(ws->pM[scale_self[n]]));
    if ((uint64_t) n == length-1) {
        upper_context->syncL(&overcarry[length], &undercarry[length]);
    }
    fmpz_add(&storage[n], &storage[n], &undercarry[n+1]);
}

void Burner_singlethreaded::recurse(int64_t n) {
    if (n < 0) return;
    uint64_t pow = 1 << scale_delta[n];
    for (uint32_t i = 0; i < pow; i++) {
        recurse(n-1);
        syncR(n);
    }
    syncL(n);
}

uint64_t Burner_singlethreaded::step() {
    recurse(length-1);
    return 1 << scale_self[length-1];
}


