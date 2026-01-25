#include <mpi.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <iostream>
#include <cassert>

#include "communicate.h"
#include "metrics.h"

void send(Metrics* metrics, int rank, int d, int64_t* x) {
    metrics->start_timer(d > 0 ? waiting_send_left : waiting_send_right);
    metrics->start_timer(d > 0 ? waiting_send_left_mpi : waiting_send_right_mpi);
    MPI_Send(x, 1, MPI_LONG, rank, 2, MPI_COMM_WORLD);
    metrics->stop_timer(d > 0 ? waiting_send_left_mpi : waiting_send_right_mpi);
    metrics->stop_timer(d > 0 ? waiting_send_left : waiting_send_right);
}

void recv(Metrics* metrics, int rank, int d, int64_t* x) {
    metrics->start_timer(d > 0 ? waiting_send_left : waiting_send_right);
    metrics->start_timer(d > 0 ? waiting_send_left_mpi : waiting_send_right_mpi);
    MPI_Recv(x, 1, MPI_LONG, rank, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    metrics->stop_timer(d > 0 ? waiting_send_left_mpi : waiting_send_right_mpi);
    metrics->stop_timer(d > 0 ? waiting_send_left : waiting_send_right);
}

void send(Metrics* metrics, int rank, int d, fmpz_t fx) {
    metrics->start_timer(d > 0 ? waiting_send_left : waiting_send_right);
    metrics->start_timer(d > 0 ? waiting_send_left_copy : waiting_send_right_copy);
    _fmpz_promote_val(fx);
    mpz_ptr x = COEFF_TO_PTR(*fx);
    const size_t size = 8; // bytes per limb?
    const size_t numb = GMP_LIMB_BITS; // nah, don't do nails
    const size_t count = (mpz_sizeinbase(x, 2) + numb-1)/numb;
    assert(count == mpz_size(x) || count == 1);
    void* buf = malloc(count*size);
    size_t countp = 0;
    mpz_export(buf, &countp, 1, size, 0, 0, x);
    assert(countp == 0 ? count == 1 : count == countp);
    metrics->stop_timer(d > 0 ? waiting_send_left_copy : waiting_send_right_copy);
    metrics->start_timer(d > 0 ? waiting_send_left_mpi : waiting_send_right_mpi);
    // TODO: named tag constants
    const int error = MPI_Send(buf, countp, MPI_LONG, rank, 1, MPI_COMM_WORLD);
    free(buf);
    metrics->stop_timer(d > 0 ? waiting_send_left_mpi : waiting_send_right_mpi);
    assert(error == 0);
    metrics->stop_timer(d > 0 ? waiting_send_left : waiting_send_right);
}

void recv(Metrics* metrics, int rank, int d, fmpz_t fx) {
    metrics->start_timer(d > 0 ? waiting_recv_left : waiting_recv_right);
    metrics->start_timer(d > 0 ? waiting_recv_left_mpi : waiting_recv_right_mpi);
    MPI_Status status;
    MPI_Probe(rank, 1, MPI_COMM_WORLD, &status);
    int count;
    MPI_Get_count(&status, MPI_LONG, &count);
    void* buf = malloc(count * sizeof(long));
    MPI_Recv(buf, count, MPI_LONG, rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    metrics->stop_timer(d > 0 ? waiting_recv_left_mpi : waiting_recv_right_mpi);
    metrics->start_timer(d > 0 ? waiting_recv_left_copy : waiting_recv_right_copy);
    const size_t size = 8;
    _fmpz_promote(fx);
    mpz_ptr x = COEFF_TO_PTR(*fx);
    mpz_import(x, static_cast<size_t>(count), 1, size, 0, 0, buf);
    free(buf);
    metrics->stop_timer(d > 0 ? waiting_recv_left_copy : waiting_recv_right_copy);
    metrics->stop_timer(d > 0 ? waiting_recv_left : waiting_recv_right);
}

void sendLeft(Context* ctx, fmpz_t x) {
    send(&ctx->metrics, ctx->task.world_rank+1, +1, x);
}
void receiveLeft(Context* ctx, fmpz_t x) {
    recv(&ctx->metrics, ctx->task.world_rank+1, +1, x);
}
void sendRight(Context* ctx, fmpz_t x) {
    send(&ctx->metrics, ctx->task.world_rank-1, -1, x);
}
void receiveRight(Context* ctx, fmpz_t x) {
    recv(&ctx->metrics, ctx->task.world_rank-1, -1, x);
}

void sendLeft(Context* ctx, timed_fmpz* x) {
    send(&ctx->metrics, ctx->task.world_rank+1, +1, &x->fmpz);
    send(&ctx->metrics, ctx->task.world_rank+1, +1, &x->iterations);
}
void receiveLeft(Context* ctx, timed_fmpz* x) {
    recv(&ctx->metrics, ctx->task.world_rank+1, +1, &x->fmpz);
    recv(&ctx->metrics, ctx->task.world_rank+1, +1, &x->iterations);
}
void sendRight(Context* ctx, timed_fmpz* x) {
    send(&ctx->metrics, ctx->task.world_rank-1, -1, &x->fmpz);
    send(&ctx->metrics, ctx->task.world_rank-1, -1, &x->iterations);
}
void receiveRight(Context* ctx, timed_fmpz* x) {
    recv(&ctx->metrics, ctx->task.world_rank-1, -1, &x->fmpz);
    recv(&ctx->metrics, ctx->task.world_rank-1, -1, &x->iterations);
}


void gather(Context* ctx, fmpz_t fitem, fmpz* buffer, int root) {
    ctx->metrics.start_timer(gather_communication);
    const int world_size = ctx->task.world_size;
    // Despite our willingness to do it, GMP, FLINT, and MPI all
    // count object sizes in `int`- the signed 32 bit integer.
    // By specifying `long`s we can get roughly 2^38 sized messages,
    // after which everything would crash hard and in unison.
    int* sizesbuf = (int*) malloc(world_size * sizeof(int));
    int* displs = (int*) malloc(world_size * sizeof(int));
    const uint64_t limb_size = 8; // bytes?
    _fmpz_promote_val(fitem);
    mpz_ptr item = COEFF_TO_PTR(*fitem);
    const uint64_t send_limb_count = mpz_size(item);
    void* sendbuf = malloc(send_limb_count*(uint64_t)limb_size);
    size_t sent_limb_count;
    // TODO: this should be incorrect, in the edge case that
    // the size allocated is slightly longer than the size used
    const int order = -1;
    mpz_export(sendbuf, &sent_limb_count, order, limb_size, 0, 0, item);
    assert(send_limb_count == sent_limb_count);
    int send_limb_count_int = static_cast<int>(send_limb_count);
    MPI_Gather(&send_limb_count_int, 1, MPI_INT, sizesbuf, 1, MPI_INT, root, MPI_COMM_WORLD);
    uint64_t* limbs = nullptr;
    if (ctx->task.world_rank == root) {
        displs[0] = 0;
        for (int i = 1; i < world_size; i++) {
            displs[i] = displs[i-1] + sizesbuf[i-1];
        }
        limbs = (uint64_t*) calloc(displs[world_size-1] + sizesbuf[world_size-1], sizeof(uint64_t));
    }
    MPI_Gatherv(sendbuf, send_limb_count_int, MPI_LONG, limbs, sizesbuf, displs, MPI_LONG, root, MPI_COMM_WORLD);
    if (ctx->task.world_rank == root) {
        for (int i = 0; i < world_size; i++) {
            fmpz* rop = &buffer[i];
            _fmpz_promote(rop);
            mpz_import(COEFF_TO_PTR(*rop), static_cast<size_t>(sizesbuf[i]), order, limb_size, 0, 0, (void*)(&limbs[displs[i]]));
        }
        free(limbs);
    }
    free(sendbuf);
    free(displs);
    free(sizesbuf);
    ctx->metrics.stop_timer(gather_communication);
}


