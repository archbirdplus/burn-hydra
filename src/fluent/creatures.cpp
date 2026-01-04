#include "fluent.h"

// Famous creatures in the Busy Beaver Challenge

// TODO: figure out sane defaults
static Setup::HydraMap() {
    return Setup()
        .block_sizes({{8,18},{18,19},{19,20},{20,21}},{{21,21,21}})
        .set_flint_threads(4)
        .consistent_collatz(3, 2, {0, 0})
        .do_prune(true)
}

// TODO: implement halting behavior without leaking scan context

static Setup::Hydra() {
    return hydraMap()
        .set_initial(3);
}

static Setup::Antihydra() {
    return hydraMap()
        .set_initial(8);
}

// TODO: settle the name for 114e12 TM
static Setup::PowerHydra() {
    return hydraMap()
        .set_initial(10)
        .iterations(114817814715809);
}

static Setup::Bigfoot() {
    // TODO: check bigfoot setup
    return Setup()
        .block_sizes({{8,18},{18,19},{19,20},{20,21}},{{21,21,21}})
        .set_flint_threads(4)
        .consistent_collatz(256, 81, {1, 2, 5, 9, 11, 15, 19, 21, 24, 27, 30, 34, 37, 39, 43, 47, 49, 53, 55, 58, 62, 66, 69, 71, 75, 77, 81, 85, 87, 90, 94, 97, 101, 104, 107, 109, 113, 115, 119, 122, 125, 129, 132, 135, 139, 141, 143, 147, 151, 154, 157, 160, 163, 167, 170, 173, 175, 179, 182, 186, 189, 192, 195, 198, 201, 205, 207, 210, 214, 217, 220, 224, 226, 229, 233, 237, 239, 242, 245, 248, 252, 255})
        .do_prune(true)
        .set_initial(5);
}


