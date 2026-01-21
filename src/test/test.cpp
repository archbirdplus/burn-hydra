#include <gtest/gtest.h>
#include <iostream>

#include "state.h"

int main(int argc, char **argv) {
    // test_parse_config();
    // test_parse_args();
    // test_get_opponent();
    // test_fluent();
    testing::InitGoogleTest(&argc, argv);
    int r = RUN_ALL_TESTS();
    std::cerr << "test main done" << std::endl;
    return r;
}

