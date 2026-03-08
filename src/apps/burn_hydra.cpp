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
    if ((ctx->even + ctx->odd) % (1<<30) == 0) {
        std::cout << "another billion: " << (ctx->even + ctx->odd) << std::endl;
    }
    return x;
}


int main(int argc, char** argv) {
    scan_fn_t count_fn = &count_parities;
    count_context_t ctx = { .even=0, .odd=0 };

    auto s = CollatzBuilder()
        .set_flint_threads(1)
        .do_prune(true)
        .from_argv(argc, argv)
        .consistent_collatz(3, 2, {0, 1})
        .initial_value(3)
        .set_table_size(16)
        .scan_fn(count_fn, 16, false)
        .scan_context(&ctx);
    Context c = s.init();
    c.run();

    std::cout << "Count after " << c.task->max_iterations << " iterations --" << std::endl;
    std::cout << "    even: " << ctx.even << std::endl;
    std::cout << "     odd: " << ctx.odd<< std::endl;

    return 0;
}

