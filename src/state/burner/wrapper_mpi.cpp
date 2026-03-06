#include "kernels.h"
#include "communicate.h"
#include <iostream>
#include <cassert>

Wrapper_MPI::Wrapper_MPI(Context* global_ctx) {
    global_context = global_ctx;
    local_burner = nullptr;
    metrics = std::unique_ptr<Metrics>(new Metrics(true));
    world_size = global_ctx->task->world_size;
    world_rank = global_ctx->task->world_rank;
    can_push_left = world_rank != world_size - 1;
    can_push_right = world_rank != 0;
    local_scales = {};
    for (uint64_t i = 0; i < global_ctx->task->block_sizes[world_rank].size(); i++) {
        local_scales.push_back(static_cast<uint32_t>(global_ctx->task->block_sizes[world_rank][i]));
    }
    local_next_scale = world_rank > 0 ?
        global_ctx->task->block_sizes[world_rank-1].back() :
        local_scales.front(); // TODO: this depends on the power of the basecase
}

Wrapper_MPI::~Wrapper_MPI() {
    
}

subscription_t Wrapper_MPI::add_subscriber(Runnable* burner) {
    if (local_burner != nullptr) {
        throw std::runtime_error("Added more than one burner to an MPI wrapper");
    }
    local_burner = burner;
    return {
        .scales = local_scales,
        .next_scale = local_next_scale,
        .can_push_right = can_push_right,
        .can_push_left = can_push_left,
        .id = 0
    };
}

void Wrapper_MPI::logs_with_prefix(std::string prefix) {
    std::string own_prefix = prefix + "_" + std::to_string(world_rank);
    metrics->dump_with_prefix(own_prefix);
    local_burner->logs_with_prefix(own_prefix);
}

void Wrapper_MPI::run_until(uint64_t end) {
    local_burner->run_until(end);
}

void Wrapper_MPI::pullR(uint64_t _, timed_fmpz* x_import) {
    assert(can_push_right);
    receiveRight(metrics.get(), world_rank, x_import);
}

void Wrapper_MPI::pushR(uint64_t _, timed_fmpz* x_export) {
    assert(can_push_right);
    sendRight(metrics.get(), world_rank, x_export);
}

void Wrapper_MPI::pushL(uint64_t _, timed_fmpz* x_export) {
    assert(can_push_left);
    sendLeft(metrics.get(), world_rank, x_export);
}

void Wrapper_MPI::pullL(uint64_t _, timed_fmpz* x_import) {
    assert(can_push_left);
    receiveLeft(metrics.get(), world_rank, x_import);
}


