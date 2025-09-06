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
    const size_t size = 8; // bytes per word?
    const size_t numb = 8*size; // bits per word
    const size_t count = (mpz_sizeinbase(x, 2) + numb-1)/numb;
    void* buf = malloc(count*size);
    size_t countp = 0;
    mpz_export(buf, &countp, 1, size, 0, 0, x);
    // std::cout << "expected: " << count << "; countp turns out: " << countp << std::endl;
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

