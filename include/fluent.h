// Fluent C++ wrapper API

#ifndef FLUENT_H
#define FLUENT_H

#include "state.h"
#include "types.h"

class Context;

// Setup stores the information about a computation before it is executed.
class Setup {
public:
    opt<collatz_function_t> collatz;
    opt<int64_t> initial;
    opt<int64_t> max_iterations;
    opt<int64_t> checkpoint_interval;
    opt<bool> prune;

    opt<uint64_t> table_size;
    opt<vecvec<uint64_t>> block_sizes_ramp;
    opt<vecvec<uint64_t>> block_sizes_plat;
    opt<int> flint_threads;

    opt<scan_config_t> scan_config;

    Setup(); // init with no defaults
    // common creatures
    static Setup hydra();
    static Setup bigfoot();
    Setup clone() const; // init new from self

    Setup& from_argv();

    Setup& block_sizes(vecvec<uint64_t> ramp_up, vecvec<uint64_t> plat);
    Setup& set_checkpoint_interval(int64_t interval);

    Setup& set_flint_threads(int threads);

    Setup& consistent_collatz(int64_t r, int64_t m, vec<int64_t> J);
    Setup& set_initial(int64_t x);
    Setup& set_iterations(int64_t n);

    Setup& do_prune(bool prune);
    Setup& set_table_size(int64_t n);

    Setup& scan_fn(scan_fn_t fn, uint64_t block_size, bool memoize);
    Setup& scan_context(void*);

    bool check() const;
    Context init() const;
};





void test_fluent();

#endif // FLUENT_H
