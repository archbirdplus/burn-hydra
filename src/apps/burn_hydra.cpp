#include "fluent/fluent.h"
#include <iostream>

typedef struct count_context {
    uint64_t even;
    uint64_t odd;
} count_context_t;

user_object_t count_parities(count_context_t* context, user_object_t x, uint64_t residue) {
    if (residue & 1) {
        context->odd += 1;
    } else {
        context->even += 1;
    }
    return x;
}


int main() {
    scan_fn_t count_fn = &count_parities;
    count_context_t ctx = .{ even=0, odd=0 };

    uint64_t iterations = 1e9;
    auto s = Setup()
        .block_sizes(.{.{8,18},.{18,19},.{19,20},.{20,21}},.{{21,21,21}})
        .set_flint_threads(4);
        .do_prune(true);
        .from_argv()
        .consistent_collatz(3, 2, .{0, 0})
        .set_initial(10)
        .set_iterations(iterations)
        .scan_fn(count_fn, 1, false)
        .scan_context(&ctx);
    s.init().run();

    std::cout << "Count after " << iterations << " iterations --" << std::endl;
    cout << "    even: " << ctx.even << std::endl;
    cout << "     odd: " << ctx.odd<< std::endl;

    return 0;
}

