#include <mpi.h>
#include <gmp.h>
#include <iostream>
#include <cassert>
#include "common.h"

void send(segment_t* segment, int rank, mpz_t x) {
    const size_t size = 8; // bytes per word?
    const size_t numb = 8*size; // bits per word
    const size_t count = (mpz_sizeinbase(x, 2) + numb-1)/numb;
    void* buf = malloc(count*size);
    size_t countp = 0;
    mpz_export(buf, &countp, 1, size, 0, 0, x);
    // std::cout << "expected: " << count << "; countp turns out: " << countp << std::endl;
    assert(count - countp >= 0);
    assert(count - countp <= 1);
    // TODO: named tag constants
    const int error = MPI_Send(buf, countp, MPI_LONG, rank, 1, MPI_COMM_WORLD);
    assert(error == 0);
}

void recv(segment_t* segment, int rank, mpz_t x) {
    MPI_Status status;
    const int err1 = MPI_Probe(rank, 1, MPI_COMM_WORLD, &status);
    int count;
    MPI_Get_count(&status, MPI_LONG, &count);
    void* buf = malloc(count * sizeof(long));
    const int err2 = MPI_Recv(buf, count, MPI_LONG, rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    const size_t size = 8;
    mpz_import(x, static_cast<size_t>(count), 1, size, 0, 0, buf);
}

void sendLeft(segment_t* segment, mpz_t x) {
    send(segment, segment->world_rank+1, x);
}
void receiveLeft(segment_t* segment, mpz_t x) {
    recv(segment, segment->world_rank+1, x);
}
void sendRight(segment_t* segment, mpz_t x) {
    send(segment, segment->world_rank-1, x);
}
void receiveRight(segment_t* segment, mpz_t x) {
    recv(segment, segment->world_rank-1, x);
}

