#include "mpi.h"
#include "state.h"
#include "fluent.h"
#include "friendly_assert.h"

Setup::Setup() {
    // everything null
}

Setup Setup::clone() const {
    Setup setup = Setup();

    if (this->block_sizes_ramp && this->block_sizes_plat)
        setup.block_sizes(*this->block_sizes_ramp, *this->block_sizes_plat);
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

Setup& Setup::from_argv() {
    // TODO: refactor argv parsing
    return *this;
}

Setup& Setup::block_sizes(vecvec<uint64_t> ramp_up, vecvec<uint64_t> plat) {
    this->block_sizes_ramp = ramp_up;
    this->block_sizes_plat = plat;
    return *this;
}
Setup& Setup::set_checkpoint_interval(int64_t interval) {
    this->checkpoint_interval = interval;
    return *this;
}

Setup& Setup::set_flint_threads(int threads) {
    this->flint_threads = threads;
    return *this;
}

Setup& Setup::consistent_collatz(int64_t r, int64_t m, vec<int64_t> J) {
    this->collatz = {
        .r = r, .m = m, .J = J
    };
    return *this;
}
Setup& Setup::set_initial(int64_t r) {
    this->initial = r;
    return *this;
}
Setup& Setup::set_iterations(int64_t r) {
    this->max_iterations = r;
    return *this;
}

Setup& Setup::do_prune(bool prune) {
    this->prune = prune;
    return *this;
}
Setup& Setup::set_table_size(int64_t n) {
    this->table_size = n;
    return *this;
}

Setup& Setup::scan_fn(scan_fn_t fn, uint64_t block_size, bool memoize) {
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
Setup& Setup::scan_context(void* context) {
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

bool Setup::check() const {
    return collatz && initial && max_iterations && checkpoint_interval && prune && table_size && block_sizes_ramp && block_sizes_plat && flint_threads && scan_config && scan_config->scan_fn;
}

Context Setup::init() const {
    return Context(this);
}

void test_optional_chain() {
    bool e = false;
    friendly_concern_equal(&e, (opt<int>{1} && std::make_optional("abc")), true, "&& all non-null");
    friendly_concern_equal(&e, (std::make_optional((char*)0) && opt<int>{1}), true, "&& all non-null");
    friendly_concern_equal(&e, (opt<int>{1} && (opt<int>)std::nullopt && std::make_optional("abc")), false, "&& nullopt not false");
    friendly_concern_equal(&e, ((opt<int>)std::nullopt && opt<int>{1} && std::make_optional("abc")), false, "&& nullopt not false");
}

void test_Setup_null() {
    Setup s = Setup();
    bool e = false;
    friendly_concern(&e, !s.collatz, "Setup::Setup() initialized collatz as not null");
    friendly_concern(&e, !s.initial, "Setup::Setup() initialized initial as not null");
    friendly_concern(&e, !s.max_iterations, "Setup::Setup() initialized max_iterations as not null");
    friendly_concern(&e, !s.checkpoint_interval, "Setup::Setup() initialized checkpoint_interval as not null");
    friendly_concern(&e, !s.prune, "Setup::Setup() initialized prune as not null");
    friendly_concern(&e, !s.table_size, "Setup::Setup() initialized table_size as not null");
    friendly_concern(&e, !s.block_sizes_ramp, "Setup::Setup() initialized block_sizes_ramp as not null");
    friendly_concern(&e, !s.block_sizes_plat, "Setup::Setup() initialized block_sizes_plat as not null");
    friendly_concern(&e, !s.flint_threads, "Setup::Setup() initialized flint_threads as not null");
    friendly_concern(&e, !s.scan_config, "Setup::Setup() initialized scan_config as not null");
    if (e) { exit(1); }
}

void test_Setup_scan_orders() {
    Setup s = Setup();
    bool e = false;
    void* ctx = (void*) 123;
    auto f = (scan_fn_t)test_Setup_scan_orders;
    s
        .scan_context(ctx)
        .scan_fn(f, 2, true);
    friendly_concern_equal(&e, s.scan_config->scan_fn, f, "scan_fn not set correctly");
    friendly_concern_equal(&e, s.scan_config->scan_context, ctx, "scan_context not set correctly");
    friendly_concern_equal(&e, s.scan_config->scan_block_size, (uint64_t)2, "scan_block_size not set correctly");
    friendly_concern_equal(&e, s.scan_config->scan_memoize, true, "scan_memoize not set correctly");
    ctx = (void*) 456;
    s.scan_context(ctx);
    friendly_concern_equal(&e, s.scan_config->scan_fn, f, "scan_fn got replaced incorrectly");
    friendly_concern_equal(&e, s.scan_config->scan_context, ctx, "scan_context not updated correctly");
    if (e) { exit(1); }
}

void test_fluent() {
    test_optional_chain();
    test_Setup_null();
    test_Setup_scan_orders();
}

