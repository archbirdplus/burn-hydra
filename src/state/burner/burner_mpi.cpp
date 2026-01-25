#include "kernels.h"
#include "communicate.h"

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

void Burner_MPI::syncR(timed_fmpz* x_export, timed_fmpz* x_import) {
    if (!can_push_right) {
        basecase_context->step(x_export, x_import);
    } else {
        sendRight(global_context, x_export);
        receiveRight(global_context, x_import);
    }
}

// TODO: should this check for what x_import *was*?
void Burner_MPI::syncL(timed_fmpz* x_export, timed_fmpz* x_import) {
    if (world_rank == world_size - 1) {
        fmpz_swap(&x_import->fmpz, &x_export->fmpz);
        int64_t tmp = x_import->iterations;
        x_export->iterations = x_import->iterations;
        x_import->iterations = tmp;
        // TODO: check
        fmpz_mul(&x_import->fmpz, &x_import->fmpz, &(global_context->workspace.pM[node_context->scale_self.back()]));
    } else {
        receiveLeft(global_context, x_import);
        sendLeft(global_context, x_export);
    }
}

uint64_t Burner_MPI::step() {
    return node_context->step();
}


