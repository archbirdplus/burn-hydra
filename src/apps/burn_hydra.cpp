#include "fluent.h"
#include <iostream>

typedef struct count_context {
    uint64_t even;
    uint64_t odd;
} count_context_t;

user_object_t count_parities(void* context, user_object_t x, uint64_t residue) {
    const auto ctx = (count_context_t*) context;
    if (residue & 1) {
        ctx->odd += 1;
    } else {
        ctx->even += 1;
    }
    return x;
}


int main() {
    scan_fn_t count_fn = &count_parities;
    count_context_t ctx = { .even=0, .odd=0 };

    uint64_t iterations = 1e9;
    auto s = Setup()
        .block_sizes({{8,18},{18,19},{19,20},{20,21}},{{21,21,21}})
        .set_flint_threads(4)
        .do_prune(true)
        .set_iterations(iterations)
        .from_argv()
        .consistent_collatz(3, 2, {0, 0})
        .set_initial(10)
        .scan_fn(count_fn, 1, false)
        .scan_context(&ctx);
    s.init().run();

    std::cout << "Count after " << iterations << " iterations --" << std::endl;
    std::cout << "    even: " << ctx.even << std::endl;
    std::cout << "     odd: " << ctx.odd<< std::endl;

    return 0;
}

