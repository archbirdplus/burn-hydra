#include "kernels.h"
#include "communicate.h"
#include <iostream>
#include <cassert>

Burner_MPI::Burner_MPI(Context* global_ctx) {
    global_context = global_ctx;
    world_size = global_ctx->task.world_size;
    world_rank = global_ctx->task.world_rank;
    can_push_left = world_rank != world_size - 1;
    can_push_right = world_rank != 0;
    local_scales = {};
    for (uint64_t i = 0; i < global_ctx->task.block_sizes[world_rank].size(); i++) {
        local_scales.push_back(static_cast<uint32_t>(global_ctx->task.block_sizes[world_rank][i]));
    }
    local_next_scale = world_rank > 0 ?
        global_ctx->task.block_sizes[world_rank-1].back() :
        local_scales.front(); // TODO: this depends on the power of the basecase
}

Burner_MPI::~Burner_MPI() {
    
}

void Burner_MPI::pullR(timed_fmpz* x_import) {
    assert(can_push_right);
    receiveRight(global_context, x_import);
}

void Burner_MPI::pushR(timed_fmpz* x_export) {
    assert(can_push_right);
    sendRight(global_context, x_export);
}

void Burner_MPI::pushL(timed_fmpz* x_export) {
    assert(can_push_left);
    sendLeft(global_context, x_export);
}

void Burner_MPI::pullL(timed_fmpz* x_import) {
    assert(can_push_left);
    receiveLeft(global_context, x_import);
}


