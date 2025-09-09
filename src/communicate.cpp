#include <mpi.h>
#include <gmp.h>
#include <iostream>
#include <cassert>

#include "common.h"
#include "segment.h"
#include "metrics.h"

void send(metrics_t* metrics, int rank, int d, mpz_t x) {
    timer_start(metrics, d > 0 ? waiting_send_left : waiting_send_right);
    timer_start(metrics, d > 0 ? waiting_send_left_copy : waiting_send_right_copy);
    const size_t size = 8; // bytes per limb?
    const size_t numb = GMP_LIMB_BITS; // nah, don't do nails
    const size_t count = (mpz_sizeinbase(x, 2) + numb-1)/numb;
    assert(count == mpz_size(x) || count == 1);
    void* buf = malloc(count*size);
    size_t countp = 0;
    mpz_export(buf, &countp, 1, size, 0, 0, x);
    assert(count - countp >= 0);
    assert(count - countp <= 1);
    timer_stop(metrics, d > 0 ? waiting_send_left_copy : waiting_send_right_copy);
    timer_start(metrics, d > 0 ? waiting_send_left_mpi : waiting_send_right_mpi);
    // TODO: named tag constants
    const int error = MPI_Send(buf, countp, MPI_LONG, rank, 1, MPI_COMM_WORLD);
    timer_stop(metrics, d > 0 ? waiting_send_left_mpi : waiting_send_right_mpi);
    assert(error == 0);
    timer_stop(metrics, d > 0 ? waiting_send_left : waiting_send_right);
}

void recv(metrics_t* metrics, int rank, int d, mpz_t x) {
    timer_start(metrics, d > 0 ? waiting_recv_left : waiting_recv_right);
    timer_start(metrics, d > 0 ? waiting_recv_left_mpi : waiting_recv_right_mpi);
    MPI_Status status;
    const int err1 = MPI_Probe(rank, 1, MPI_COMM_WORLD, &status);
    int count;
    MPI_Get_count(&status, MPI_LONG, &count);
    void* buf = malloc(count * sizeof(long));
    const int err2 = MPI_Recv(buf, count, MPI_LONG, rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    timer_stop(metrics, d > 0 ? waiting_recv_left_mpi : waiting_recv_right_mpi);
    timer_start(metrics, d > 0 ? waiting_recv_left_copy : waiting_recv_right_copy);
    const size_t size = 8;
    mpz_import(x, static_cast<size_t>(count), 1, size, 0, 0, buf);
    timer_stop(metrics, d > 0 ? waiting_recv_left_copy : waiting_recv_right_copy);
    timer_stop(metrics, d > 0 ? waiting_recv_left : waiting_recv_right);
}

void sendLeft(data_t* data, mpz_t x) {
    send(data->metrics, data->segment->world_rank+1, +1, x);
}
void receiveLeft(data_t* data, mpz_t x) {
    recv(data->metrics, data->segment->world_rank+1, +1, x);
}
void sendRight(data_t* data, mpz_t x) {
    send(data->metrics, data->segment->world_rank-1, -1, x);
}
void receiveRight(data_t* data, mpz_t x) {
    recv(data->metrics, data->segment->world_rank-1, -1, x);
}


void gather(data_t* data, mpz_t item, mpz_ptr* buffer, int root) {
    const int world_size = data->segment->world_size;
    // timer
    // Despite our willingness to do it, both GMP and MPI
    // count object sizes in `int`- the signed 32 bit integer.
    // By specifying `long`s we can get roughly 2^38 sizes messages,
    // after which everything would crash hard and in unison.
    int* sizesbuf = (int*) malloc(world_size * sizeof(int));
    int* displs = (int*) malloc(world_size * sizeof(int));
    const uint64_t limb_size = 8; // bytes?
    const uint64_t send_limb_count = mpz_size(item);
    void* sendbuf = malloc(send_limb_count*(uint64_t)limb_size);
    size_t sent_limb_count;
    // timer
    // TODO: this should be incorrect, in the edge case that
    // the size allocated is slightly longer than the size used
    const int order = -1;
    mpz_export(sendbuf, &sent_limb_count, order, limb_size, 0, 0, item);
    assert(send_limb_count == sent_limb_count);
    // timer
    int send_limb_count_int = static_cast<int>(send_limb_count);
    MPI_Gather(&send_limb_count_int, 1, MPI_INT, sizesbuf, 1, MPI_INT, root, MPI_COMM_WORLD);
    uint64_t* limbs;
    if (data->segment->world_rank == root) {
        displs[0] = 0;
        for (int i = 1; i < world_size; i++) {
            displs[i] = displs[i-1] + sizesbuf[i-1];
        }
        limbs = (uint64_t*) calloc(displs[world_size-1] + sizesbuf[world_size-1], sizeof(uint64_t));
    }
    MPI_Gatherv(sendbuf, send_limb_count_int, MPI_LONG, limbs, sizesbuf, displs, MPI_LONG, root, MPI_COMM_WORLD);
    // timer
    if (data->segment->world_rank == root) {
        for (int i = 0; i < world_size; i++) {
            mpz_ptr rop = buffer[i];
            mpz_import(rop, static_cast<size_t>(sizesbuf[i]), order, limb_size, 0, 0, (void*)(&limbs[displs[i]]));
        }
        free(limbs);
    }
    free(sendbuf);
    free(displs);
    free(sizesbuf);
    // timer
}


