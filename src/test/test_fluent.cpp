#include <gtest/gtest.h>
#include <optional>

namespace Collatz {
    #include "state.h"
}

template<class T>
using opt = std::optional<T>;

TEST(FluentTest, OptionalChain) {
    EXPECT_EQ(opt<int>{1} && std::make_optional("abc"), true) << "&& all non-null";
    EXPECT_EQ((std::make_optional((char*)0) && opt<int>{1}), true) << "&& all non-null";
    EXPECT_EQ((opt<int>{1} && (opt<int>)std::nullopt && std::make_optional("abc")), false) << "&& nullopt not false";
    EXPECT_EQ(((opt<int>)std::nullopt && opt<int>{1} && std::make_optional("abc")), false) << "&& nullopt not false";
    EXPECT_EQ(1, 2);
}

/*
TEST(FluentTest, SetupNull) {
    Collatz::Setup s = Collatz::Setup();
    EXPECT_TRUE(!s.collatz) << "Setup::Setup() initialized collatz as not null";
    EXPECT_TRUE(!s.initial) << "Setup::Setup() initialized initial as not null";
    EXPECT_TRUE(!s.max_iterations) << "Setup::Setup() initialized max_iterations as not null";
    EXPECT_TRUE(!s.checkpoint_interval) << "Setup::Setup() initialized checkpoint_interval as not null";
    EXPECT_TRUE(!s.prune) << "Setup::Setup() initialized prune as not null";
    EXPECT_TRUE(!s.table_size) << "Setup::Setup() initialized table_size as not null";
    EXPECT_TRUE(!s.block_sizes_ramp) << "Setup::Setup() initialized block_sizes_ramp as not null";
    EXPECT_TRUE(!s.block_sizes_plat) << "Setup::Setup() initialized block_sizes_plat as not null";
    EXPECT_TRUE(!s.flint_threads) << "Setup::Setup() initialized flint_threads as not null";
    EXPECT_TRUE(!s.scan_config) << "Setup::Setup() initialized scan_config as not null";
}

void foo() { }

TEST(FluentTest, OverwriteScan) {
    Collatz::Setup s = Collatz::Setup();
    void* ctx = (void*) 123;
    auto f = (Collatz::scan_fn_t)foo;
    s
        .scan_context(ctx)
        .scan_fn(f, 2, true);
    EXPECT_EQ(s.scan_config->scan_fn, f) << "scan_fn not set correctly";
    EXPECT_EQ(s.scan_config->scan_context, ctx) << "scan_context not set correctly";
    EXPECT_EQ(s.scan_config->scan_block_size, (uint64_t)2) << "scan_block_size not set correctly";
    EXPECT_EQ(s.scan_config->scan_memoize, true) << "scan_memoize not set correctly";
    ctx = (void*) 456;
    s.scan_context(ctx);
    EXPECT_EQ(s.scan_config->scan_fn, f) << "scan_fn got replaced incorrectly";
    EXPECT_EQ(s.scan_config->scan_context, ctx) << "scan_context not updated correctly";
}
*/



