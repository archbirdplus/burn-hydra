#include "mpi.h"
#include "fluent.h"
#include "friendly_assert.h"

bool Setup::check() const {
    return initial && iterations && checkpoint_interval && prune && block_sizes_ramp && block_sizes_plat && flint_threads && scan_config;
}

void test_optional_chain() {
    bool e = false;
    friendly_concern_equal(&e, (opt<int>{1} && std::make_optional("abc")), true, "&& all non-null");
    friendly_concern_equal(&e, (std::make_optional((char*)0) && opt<int>{1}), true, "&& all non-null");
    friendly_concern_equal(&e, (opt<int>{1} && (opt<int>)std::nullopt && std::make_optional("abc")), false, "&& nullopt not false");
    friendly_concern_equal(&e, ((opt<int>)std::nullopt && opt<int>{1} && std::make_optional("abc")), false, "&& nullopt not false");
}

void test_fluent() {
    test_optional_chain();
}

