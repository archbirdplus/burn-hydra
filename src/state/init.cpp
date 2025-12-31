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
    static_assert(std::numeric_limits<T>::is_integer());
    static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<uint64_t>::max());
    const uint64_t table_max = std::numeric_limits<T>::max();
    const int64_t table_min = std::numeric_limits<T>::min();

    // [collatz.] x =>    r  * (x /  m ) +        J      [x %  m ]
    //            x =>^n r^n * (x / m^n) + basecase_table[x % m^n]
    // Which means that basecase_table is simply g^n of (0..<m^n).
    uint64_t N = n_pow(collatz.m, iterations);
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
            fmpz_add_si(x, x, g.J[j_ui]);
        }
        friendly_assert(fmpz_cmp_ui(x, table_max) <= 0, "Basecase table type cannot fit such large entries");
        friendly_assert(fmpz_cmp_si(x, table_min) >= 0, "Basecase table type cannot fit such negative entries");
        if (std::numeric_limits<T>::is_signed()) {
            table[s] = fmpz_get_si(x);
        } else {
            table[s] = fmpz_get_ui(x);
        }
    }
    return 
}

State::State(Setup setup) {
    ensure_MPI_init();
    workspace.basecase_table = create_basecase_table(meta.collatz);
}

void State::run() {
    // aggressive specialization for base-2 r/m, thread/node counts, table size/type, scan styles
}

