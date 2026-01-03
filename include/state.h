#ifndef STATE_H
#define STATE_H

#include <vector>
#include <cstdint>
#include "flint/flint.h"
#include "flint/fmpz.h"

#include "types.h"
#include "fluent.h"

#include "metrics.h"

class Setup;

// A fully specified Collatz task to be computed.
class Task {
public:
    collatz_function_t collatz;
    int64_t initial;
    int64_t max_iterations;
    opt<int64_t> checkpoint_interval;
    uint64_t table_size;

    vecvec<uint64_t> block_sizes;
    uint64_t block_size_max;

    int world_size;
    int world_rank;
    int flint_threads;

    opt<scan_config_t> scan_config;

    Task(Setup setup);
};

// Acceleration structures for a task.
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

    Workspace(Setup setup, Task task);
};

// Context handles the memory needed at the time of execution: a task and its structures.
class Context {
private:
public:
    Task task;
    Workspace workspace;
    Metrics metrics;

    Context(Setup setup); // init from problem statement

    void run();
};

#endif // STATE_H

