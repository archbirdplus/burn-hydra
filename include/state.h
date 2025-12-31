#ifndef STATE_H
#define STATE_H

#include <vector>
#include <cstdint>
#include "flint/flint.h"
#include "flint/fmpz.h"

#include "fluent.h"

template <class T>
using vec = std::vector<T>;
template <class T>
using vecvec = std::vector<std::vector<T>>;
template <class T>
using opt = std::optional<T>;

using user_object_t = uint64_t;
using scan_fn_t = user_object_t(*)(void*, user_object_t, uint64_t);

typedef struct scan_config {
    scan_fn_t scan_fn;
    void* scan_context;
    uint64_t scan_block_size;
    bool scan_memoize;
} scan_config_t;

// Distinct from the definition in https://wiki.bbchallenge.org/wiki/Consistent_Collatz
// Notably closer aligned to the inductive pattern style.
// x_(n+1) = r floor(x/m) + J[x_n mod m]
typedef struct collatz_function {
    int64_t r;
    int64_t m;
    vec<int64_t> J;
} collatz_function_t;


class MetaState {
public:
    collatz_function_t collatz;
    int64_t initial;
    int64_t max_iterations;
    int64_t checkpoint_interval;

    vecvec<uint64_t> block_sizes;
    uint64_t block_size_max;

    int world_size;
    int world_rank;
    int flint_threads;

    scan_config_t scan_config;

    MetaState(int64_t initial, int64_t iterations, int64_t checkpoint_interval, vecvec<uint64_t> block_sizes, scan_config_t scan_config);
};

class Workspace {
public:
    vec<fmpz> pR;
    vec<fmpz> pM;
    vec<fmpz> stored;
    void* basecase_table;

    // Table memoizing g^n in terms of indices
    vec<uint64_t> scan_table;
    // user object -> table index
    std::unordered_map<user_object_t, uint64_t> scan_index_from_object;
    // table index -> user object
    vec<uint64_t> scan_object_from_index;
};

// State handles the memory needed at the time of execution.
class State {
private:
public:
    // Setup setup; // TODO: likely not needed
    MetaState meta;
    Workspace workspace;

    State(Setup setup); // init from problem statement

    void run();
};

#endif // STATE_H

