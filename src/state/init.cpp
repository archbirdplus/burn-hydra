#include "mpi.h"
#include "flint/ulong_extras.h"

#include <algorithm>

#include "state.h"
#include "friendly_assert.h"
#include <limits>

void ensure_MPI_init() {
    int flag;
    MPI_Initialized(&flag);
    if (!flag) {
        MPI_Init(NULL, NULL);
    }
}

// Can be specialized into 32-bit and 64-bit tables.
template <class T>
T* create_basecase_table(collatz_function_t g, uint64_t n) {
    static_assert(std::numeric_limits<T>::is_integer);
    static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<uint64_t>::max());
    const uint64_t table_max = std::numeric_limits<T>::max();
    const int64_t table_min = std::numeric_limits<T>::min();

    // [collatz.] x =>    r  * (x /  m ) +        J      [x %  m ]
    //            x =>^n r^n * (x / m^n) + basecase_table[x % m^n]
    // Which means that basecase_table is simply g^n of (0..<m^n).
    uint64_t N = n_pow(g.m, n);
    T* table = (T*) malloc (N * sizeof (T));

    fmpz_t m; fmpz_init_set_si(m, g.m);

    fmpz_t x; fmpz_init(x);
    fmpz_t rem; fmpz_init(rem);
    for (uint64_t s = 0; s < N; s++) {
        fmpz_set_ui(x, s);
        for (uint64_t i = 0; i < n; i++) {
            fmpz_fdiv_qr(x, rem, x, m);
            uint64_t rem_ui = fmpz_get_ui(rem);
            int64_t j = g.J[rem_ui];
            fmpz_mul_si(x, x, g.r);
            fmpz_add_si(x, x, g.J[rem_ui]);
        }
        friendly_assert(fmpz_cmp_ui(x, table_max) <= 0, "Basecase table type cannot fit such large entries");
        friendly_assert(fmpz_cmp_si(x, table_min) >= 0, "Basecase table type cannot fit such negative entries");
        if (std::numeric_limits<T>::is_signed) {
            table[s] = fmpz_get_si(x);
        } else {
            table[s] = fmpz_get_ui(x);
        }
    }
    return table;
}

Task::Task(Setup setup) {
    bool error = false;
    bool* e = &error;
    checkpoint_interval = setup.checkpoint_interval;
    if(setup.collatz.has_value()) collatz = *setup.collatz;
    else friendly_concern(e, false, "Missing setup: collatz function");
    if(setup.initial.has_value()) initial = *setup.initial;
    else friendly_concern(e, false, "Missing setup: initial value");
    if(setup.max_iterations.has_value()) max_iterations = *setup.max_iterations;
    else friendly_concern(e, false, "Missing setup: max iterations");
    if(setup.max_iterations.has_value()) table_size = *setup.table_size;
    else friendly_concern(e, false, "Missing setup: table size");

    ensure_MPI_init();
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // TODO: automatically configure block sizes based on problem size, node count
    // TODO: check valid block sizes
    if(setup.block_sizes_ramp.has_value()) {
        vecvec<uint64_t> ramp = *setup.block_sizes_ramp;
        block_sizes = ramp;
        if(world_size > block_sizes.size() && setup.block_sizes_plat.has_value()) {
            vecvec<uint64_t> plat = *setup.block_sizes_plat;
            int ramp_size = ramp.size();
            int plat_size = plat.size();
            for(int i = ramp_size; i < world_size; i++) {
                block_sizes.push_back(plat[(i - ramp_size) % plat_size]);
            }
        } else friendly_concern(e, false, "Missing setup: block sizes plateau");
    } else friendly_concern(e, false, "Missing setup: block sizes");

    flint_threads = setup.flint_threads.value_or(1);
    scan_config = setup.scan_config;

    if(e) {
        // return std::nullopt;
        throw std::runtime_error("Configuration is missing information");
    }
}

vec<fmpz> create_2exp_powers(uint64_t r, uint64_t n) {
    // Create vector of repeated squares r^2^k for k inclusive of [0, n]
    vec<fmpz> pR = {}; // begin with r^2^0 = r^1
    {
        fmpz next; fmpz_init_set_ui(&next, r);
        pR.push_back(r);
    }
    fmpz_t tmp; fmpz_init_set_ui(tmp, r);
    for(uint64_t i = 0; i < n; i++) {
        fmpz_mul(tmp, tmp, tmp);
        fmpz next; fmpz_init_set(&next, tmp);
        pR.push_back(next);
    }
    return pR;
}

Workspace::Workspace(Setup setup, Task task) {
    vec<uint64_t> my_blocks = task.block_sizes[task.world_rank];
    uint64_t largest_size = std::max_element(my_blocks.begin(), my_blocks.end())[0];
    pR = create_2exp_powers(task.collatz.r, largest_size);
    pM = create_2exp_powers(task.collatz.m, largest_size);
    // TODO: automatically configure table size
    basecase_table = create_basecase_table<uint64_t>(task.collatz, task.table_size);
}

// TODO: really nasty constructor
Context::Context(Setup setup) : task(setup), workspace(setup, task), metrics(true) { }

void Context::run() {
    // TODO: aggressive specialization for:
    // base-2 r/m, thread/node counts, table size/type, scan styles
}

