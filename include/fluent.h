// Fluent C++ wrapper API

#pragma once

#include "state.h"
#include "types.h"

class Context;

// CollatzBuilder stores the information about a computation before it is executed.
class CollatzBuilder {
public:
    opt<collatz_function_t> collatz;
    opt<int64_t> initial;
    opt<int64_t> max_iterations;
    opt<int64_t> checkpoint_interval;
    opt<bool> prune;

    opt<uint64_t> table_size;
    opt<vecvec<uint64_t>> block_sizes_ramp;
    opt<vecvec<uint64_t>> block_sizes_plat;
    opt<vecvec<uint64_t>> thread_breaks;
    opt<int> flint_threads;

    opt<scan_config_t> scan_config;

    CollatzBuilder(); // init with no defaults
    // common creatures
    static CollatzBuilder hydra();
    static CollatzBuilder bigfoot();
    CollatzBuilder clone() const; // init new from self

    CollatzBuilder& from_argv();
    CollatzBuilder& layout_string(std::string);

    CollatzBuilder& set_layout(vecvec<uint64_t> ramp, vecvec<uint64_t> plat, vecvec<uint64_t> breaks);
    CollatzBuilder& set_thread_breaks(vecvec<uint64_t> breaks);
    CollatzBuilder& block_sizes(vecvec<uint64_t> ramp, vecvec<uint64_t> plat);
    CollatzBuilder& set_checkpoint_interval(int64_t interval);

    CollatzBuilder& set_flint_threads(int threads);

    CollatzBuilder& consistent_collatz(int64_t r, int64_t m, vec<int64_t> J);
    CollatzBuilder& set_initial(int64_t x);
    CollatzBuilder& set_iterations(int64_t n);

    CollatzBuilder& do_prune(bool prune);
    CollatzBuilder& set_table_size(int64_t n);

    CollatzBuilder& scan_fn(scan_fn_t fn, uint64_t block_size, bool memoize);
    CollatzBuilder& scan_context(void*);

    bool check() const;
    Context init() const;
};





void test_fluent();

