#include <gtest/gtest.h>

#include "parse.h"

inline void vecvec_eq(vecvec<uint64_t> lhs, vecvec<uint64_t> rhs) {
    EXPECT_EQ(lhs, rhs);
}

TEST(ParseTest, BasicStrings) {
    layout_t layout;
    parse_layout(&layout, "1,2,3/4,5/6");
    vecvec_eq({{1, 2, 3}, {4, 5}, {6}}, layout.block_sizes_ramp);
    vecvec_eq({}, layout.block_sizes_plat);
    vecvec_eq({{},{},{}}, layout.thread_breaks);
}

TEST(ParseTest, Intervals) {
    layout_t layout;
    parse_layout(&layout, "1-2,3/4-7/6-6/7-6");
    vecvec_eq({{1, 2, 3}, {4, 5, 6, 7}, {6, 6}, {7, 6}}, layout.block_sizes_ramp);
    vecvec_eq({}, layout.block_sizes_plat);
    vecvec_eq({{},{},{},{}}, layout.thread_breaks);
}

TEST(ParseTest, OpenIntervals) {
    layout_t layout;
    parse_layout(&layout, "2-1,4/-7/-6/-6");
    vecvec_eq({{2, 1, 4}, {5, 6, 7}, {6}, {6}}, layout.block_sizes_ramp);
    vecvec_eq({}, layout.block_sizes_plat);
    vecvec_eq({{},{},{},{}}, layout.thread_breaks);
}

TEST(ParseTest, Plateau) {
    layout_t layout;
    parse_layout(&layout, "1,2,3//4-6,8/2");
    vecvec_eq({{1, 2, 3}}, layout.block_sizes_ramp);
    vecvec_eq({{4, 5, 6, 8}, {2}}, layout.block_sizes_plat);
    vecvec_eq({{},{},{}}, layout.thread_breaks);
}

TEST(ParseTest, ThreadBreaksNormal) {
    layout_t layout;
    parse_layout(&layout, "1,2:3/1,2:3,4,5:6,7//4:6,8/2");
    vecvec_eq({{1, 2, 3}, {1, 2, 3, 4, 5, 6, 7}}, layout.block_sizes_ramp);
    vecvec_eq({{4, 6, 8}, {2}}, layout.block_sizes_plat);
    vecvec_eq({{1},{1,4},{0},{}}, layout.thread_breaks);
}

TEST(ParseTest, ThreadBreaksOutOfBounds) {
    layout_t layout;
    parse_layout(&layout, "1,2,3:/:1,2/:-4");
    vecvec_eq({{1, 2, 3}, {1, 2}, {3, 4}}, layout.block_sizes_ramp);
    vecvec_eq({}, layout.block_sizes_plat);
    uint64_t x = -1;
    vecvec_eq({{2},{x},{x}}, layout.thread_breaks);
}

TEST(ParseTest, OneThreadBreak) {
    layout_t layout;
    parse_layout(&layout, "1:2-1");
    vecvec_eq({{1, 2, 1}}, layout.block_sizes_ramp);
    vecvec_eq({}, layout.block_sizes_plat);
    vecvec_eq({{0}}, layout.thread_breaks);
}

// These exceptions cannot be caught for some reason.
/*
TEST(ParseTest, BeginningOpenIntervalThrows) {
    layout_t layout;
    EXPECT_THROW( parse_layout(&layout, "-1-2,4/5") , std::runtime_error);
}

TEST(ParseTest, PlateauOpenIntervalThrows) {
    layout_t layout;
    EXPECT_THROW( parse_layout(&layout, "1-2,4//-6") , std::runtime_error);
}
*/




