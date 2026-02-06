#include <gtest/gtest.h>
#include "types.h"
#include "init.h"

TEST(BasecaseTableTests, Hydra) {
    collatz_function_t g = { .r = 3, .m = 2, .J = {0, 1} };
    auto table = create_basecase_table<uint64_t>(g, 2);
    EXPECT_EQ(0, table[0]);
    EXPECT_EQ(1, table[1]);
    EXPECT_EQ(4, table[2]);
    EXPECT_EQ(6, table[3]);
}

TEST(BasecaseTableTests, WonkyHydra) {
    collatz_function_t g = { .r = 3, .m = 2, .J = {1, 3} };
    auto table = create_basecase_table<uint64_t>(g, 2);
    EXPECT_EQ(3, table[0]);
    EXPECT_EQ(6, table[1]);
    EXPECT_EQ(7, table[2]);
    EXPECT_EQ(10, table[3]);
}

TEST(BasecaseTableTests, SignedThing) {
    collatz_function_t g = { .r = 7, .m = 4, .J = {-7, 3, -6, 5} };
    auto table = create_basecase_table<int64_t>(g, 2);
    EXPECT_EQ(-11, table[0]);
    EXPECT_EQ(  5, table[1]);
    EXPECT_EQ(-20, table[2]);
    EXPECT_EQ( 10, table[3]);
    EXPECT_EQ( -7, table[4]);
    EXPECT_EQ(  8, table[5]);
    EXPECT_EQ(  3, table[6]);
    EXPECT_EQ( 14, table[7]);
    EXPECT_EQ( 12, table[8]);
    EXPECT_EQ( 31, table[9]);
    EXPECT_EQ(  7, table[10]);
    EXPECT_EQ( 33, table[11]);
    EXPECT_EQ( 15, table[12]);
    EXPECT_EQ( 35, table[13]);
    EXPECT_EQ( 26, table[14]);
    EXPECT_EQ( 36, table[15]);
}

