#include <mpi.h>

#include "friendly_assert.h"

#include "latencies.h"

int main() {
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    latencies_config_t config = {
        .sizes = {16, 24, 30},
        .counts = {4096, 64, 1},
    };

    vec<latency_matrix> stats = gather_stats(&config, world_rank, world_size);

    print_stats(&config, world_rank, world_size, stats);

    MPI_Finalize();
}


