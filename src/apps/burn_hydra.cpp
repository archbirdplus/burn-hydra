#include "fluent.h"
#include <iostream>

typedef struct count_context {
    uint64_t even;
    uint64_t odd;
} count_context_t;

user_object_t count_parities(void* context, user_object_t x, uint64_t residue) {
    const auto ctx = (count_context_t*) context;
    for (int i = 0; i < 16; i++) {
        if (residue & 1) {
            ctx->odd += 1;
        } else {
            ctx->even += 1;
        }
        residue = residue*3/2;
    }
    return x;
}


int main() {
    scan_fn_t count_fn = &count_parities;
    count_context_t ctx = { .even=0, .odd=0 };

    uint64_t iterations = 1<<30;
    auto s = CollatzBuilder()
        .block_sizes({{8,9,10,11,12,13,14,15},{16,17},{18,19},{20,21},{22,23},{24,25},{26,27}}, {})
        .set_flint_threads(1)
        .do_prune(true)
        .set_iterations(iterations)
        .from_argv()
        .consistent_collatz(3, 2, {0, 1})
        .set_initial(3)
        .set_table_size(16)
        .scan_fn(count_fn, 16, false)
        .scan_context(&ctx);
    s.init().run();

    std::cout << "Count after " << iterations << " iterations --" << std::endl;
    std::cout << "    even: " << ctx.even << std::endl;
    std::cout << "     odd: " << ctx.odd<< std::endl;

    return 0;
}

