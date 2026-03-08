#include <gtest/gtest.h>
#include <optional>

#include "state.h"

inline void optvecvec_eq(vecvec<uint64_t> lhs, opt<vecvec<uint64_t>> rhs) {
    EXPECT_FALSE(std::nullopt == rhs);
    EXPECT_EQ(lhs, *rhs);
}

TEST(FluentTest, OptionalChain) {
    EXPECT_EQ(opt<int>{1} && std::make_optional("abc"), true) << "&& all non-null";
    EXPECT_EQ((std::make_optional((char*)0) && opt<int>{1}), true) << "&& all non-null";
    EXPECT_EQ((opt<int>{1} && (opt<int>)std::nullopt && std::make_optional("abc")), false) << "&& nullopt not false";
    EXPECT_EQ(((opt<int>)std::nullopt && opt<int>{1} && std::make_optional("abc")), false) << "&& nullopt not false";
}

TEST(FluentTest, SetupNull) {
    CollatzBuilder s = CollatzBuilder();
    EXPECT_TRUE(!s.collatz) << "CollatzBuilder::CollatzBuilder() initialized collatz as not null";
    EXPECT_TRUE(!s.initial) << "CollatzBuilder::CollatzBuilder() initialized initial as not null";
    EXPECT_TRUE(!s.max_iterations) << "CollatzBuilder::CollatzBuilder() initialized max_iterations as not null";
    EXPECT_TRUE(!s.checkpoint_interval) << "CollatzBuilder::CollatzBuilder() initialized checkpoint_interval as not null";
    EXPECT_TRUE(!s.prune) << "CollatzBuilder::CollatzBuilder() initialized prune as not null";
    EXPECT_TRUE(!s.table_size) << "CollatzBuilder::CollatzBuilder() initialized table_size as not null";
    EXPECT_TRUE(!s.block_sizes_ramp) << "CollatzBuilder::CollatzBuilder() initialized block_sizes_ramp as not null";
    EXPECT_TRUE(!s.block_sizes_plat) << "CollatzBuilder::CollatzBuilder() initialized block_sizes_plat as not null";
    EXPECT_TRUE(!s.flint_threads) << "CollatzBuilder::CollatzBuilder() initialized flint_threads as not null";
    EXPECT_TRUE(!s.scan_config) << "CollatzBuilder::CollatzBuilder() initialized scan_config as not null";
}

void foo() { }

TEST(FluentTest, OverwriteScan) {
    CollatzBuilder s = CollatzBuilder();
    void* ctx = (void*) 123;
    auto f = (scan_fn_t)foo;
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

TEST(FluentTest, LayoutString) {
    CollatzBuilder s = CollatzBuilder();
    s.layout_string("1,2,3/4,5,6//7:7:7");
    optvecvec_eq({{1, 2, 3}, {4, 5, 6}}, s.block_sizes_ramp);
    optvecvec_eq({{7, 7, 7}}, s.block_sizes_plat);
    optvecvec_eq({{}, {}, {0,1}}, s.thread_breaks);
}



