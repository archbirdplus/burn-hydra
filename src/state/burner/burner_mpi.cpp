#include "kernels.h"
#include "communicate.h"
#include <iostream>

Burner_MPI::Burner_MPI(Context* global_ctx) {
    global_context = global_ctx;
    world_size = global_ctx->task.world_size;
    world_rank = global_ctx->task.world_rank;
    can_push_left = world_rank != world_size - 1;
    can_push_right = world_rank != 0;
    std::cout << "can push right: " << can_push_right << " by world rank is " << world_rank << std::endl;
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

void Burner_MPI::pullR(timed_fmpz* x_import) {
    if (!can_push_right) {
        basecase_context->pushL(x_import);
    } else {
        receiveRight(global_context, x_import);
    }
}

void Burner_MPI::pushR(timed_fmpz* x_export) {
    if (!can_push_right) {
        std::cout << "stepping now" << std::endl;
        basecase_context->step(x_export);
    } else {
        sendRight(global_context, x_export);
    }
}

void Burner_MPI::pushL(timed_fmpz* x_export) {
    assert(can_push_left);
    sendLeft(global_context, x_export);
}

// TODO: should this check for what x_import *was*?
void Burner_MPI::pullL(timed_fmpz* x_import) {
    assert(can_push_left);
    receiveLeft(global_context, x_import);
}

uint64_t Burner_MPI::step() {
    return node_context->step();
}


