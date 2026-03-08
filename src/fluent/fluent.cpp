#include "mpi.h"
#include "state.h"
#include "fluent.h"
#include "friendly_assert.h"
#include "parse.h"

CollatzBuilder::CollatzBuilder() {
    // everything null
}

CollatzBuilder CollatzBuilder::clone() const {
    CollatzBuilder setup = CollatzBuilder();

    if (this->block_sizes_ramp && this->block_sizes_plat)
        setup.block_sizes(*this->block_sizes_ramp, *this->block_sizes_plat);
    if (this->thread_breaks)
        setup.set_thread_breaks(*this->thread_breaks);
    if (this->checkpoint_interval)
        setup.set_checkpoint_interval(*this->checkpoint_interval);

    if (this->flint_threads)
        setup.set_flint_threads(*this->flint_threads);

    if (this->collatz) {
        collatz_function_t g = *this->collatz;
        setup.consistent_collatz(g.r, g.m, g.J);
    }
    if (this->initial)
        setup.set_initial(*this->initial);
    if (this->max_iterations)
        setup.set_iterations(*this->max_iterations);

    if (this->prune)
        setup.do_prune(*this->prune);
    if (this->table_size)
        setup.set_table_size(*this->table_size); // TODO: may it be non-2exp? interacts with block sizes

    if (this->scan_config) {
        scan_config_t scan = *this->scan_config;
        setup.scan_fn(scan.scan_fn, scan.scan_block_size, scan.scan_memoize);
        if (scan.scan_context)
            setup.scan_context(scan.scan_context);
    }

    return setup;
}

CollatzBuilder& CollatzBuilder::from_argv() {
    // TODO: refactor argv parsing
    return *this;
}

CollatzBuilder& CollatzBuilder::layout_string(std::string layout_string) {
    layout_t layout;
    parse_layout(&layout, layout_string.c_str());
    this->set_layout(layout.block_sizes_ramp, layout.block_sizes_plat, layout.thread_breaks);
    return *this;
}

CollatzBuilder& CollatzBuilder::set_layout(vecvec<uint64_t> ramp, vecvec<uint64_t> plat, vecvec<uint64_t> breaks) {
    this->block_sizes(ramp, plat);
    this->set_thread_breaks(breaks);
    return *this;
}

CollatzBuilder& CollatzBuilder::set_thread_breaks(vecvec<uint64_t> breaks) {
    this->thread_breaks = breaks;
    return *this;
}

CollatzBuilder& CollatzBuilder::block_sizes(vecvec<uint64_t> ramp, vecvec<uint64_t> plat) {
    this->block_sizes_ramp = ramp;
    this->block_sizes_plat = plat;
    return *this;
}

CollatzBuilder& CollatzBuilder::set_checkpoint_interval(int64_t interval) {
    this->checkpoint_interval = interval;
    return *this;
}

CollatzBuilder& CollatzBuilder::set_flint_threads(int threads) {
    this->flint_threads = threads;
    return *this;
}

CollatzBuilder& CollatzBuilder::consistent_collatz(int64_t r, int64_t m, vec<int64_t> J) {
    this->collatz = {
        .r = r, .m = m, .J = J
    };
    return *this;
}
CollatzBuilder& CollatzBuilder::set_initial(int64_t r) {
    this->initial = r;
    return *this;
}
CollatzBuilder& CollatzBuilder::set_iterations(int64_t r) {
    this->max_iterations = r;
    return *this;
}

CollatzBuilder& CollatzBuilder::do_prune(bool prune) {
    this->prune = prune;
    return *this;
}
CollatzBuilder& CollatzBuilder::set_table_size(int64_t n) {
    this->table_size = n;
    return *this;
}

CollatzBuilder& CollatzBuilder::scan_fn(scan_fn_t fn, uint64_t block_size, bool memoize) {
    void* prev_context = (void*) 0;
    if (this->scan_config) {
        prev_context = this->scan_config->scan_context;
    }
    this->scan_config = {
        .scan_fn = fn,
        .scan_context = prev_context,
        .scan_block_size = block_size,
        .scan_memoize = memoize
    };
    return *this;
}
CollatzBuilder& CollatzBuilder::scan_context(void* context) {
    if (!this->scan_config) {
        this->scan_config = {
            .scan_fn = (scan_fn_t)0,
            .scan_context = context,
            .scan_block_size = 0,
            .scan_memoize = false,
        };
    } else {
        this->scan_config->scan_context = context;
    }
    return *this;
}

bool CollatzBuilder::check() const {
    return collatz && initial && max_iterations && checkpoint_interval && prune && table_size && block_sizes_ramp && block_sizes_plat && flint_threads && scan_config && scan_config->scan_fn;
}

Context CollatzBuilder::init() const {
    return Context(this);
}


