#include "fluent.h"
#include <iostream>
#include <fstream>

struct count_state {
    uint64_t even;
    uint64_t odd;
};

typedef struct count_context {
    count_state state;
    vec<count_state> history;
} count_context_t;

user_object_t count_parities(void* context, user_object_t x, uint64_t residue) {
    const auto ctx = (count_context_t*) context;
    for (int i = 0; i < 16; i++) {
        if (residue & 1) {
            ctx->state.odd += 1;
        } else {
            ctx->state.even += 1;
        }
        residue = residue*3/2;
    }
    uint64_t steps = ctx->state.odd + ctx->state.even;
    if (steps % (1<<20) == 0) {
        ctx->history.push_back(ctx->state);
    }
    if (steps % (1<<20) == 0) {
        std::cout << "completed another million: " << (steps/(1<<20)) << std::endl;
    }
    return x;
}


int main(int argc, char** argv) {
    scan_fn_t count_fn = &count_parities;
    count_context_t ctx = { .state={ .even=0, .odd=0 }, .history={} };

    auto s = CollatzBuilder()
        .set_flint_threads(1)
        .do_prune(true)
        .from_argv(argc, argv)
        .consistent_collatz(3, 2, {0, 1})
        .set_table_size(16)
        .scan_fn(count_fn, 16, false)
        .scan_context(&ctx);
    Context c = s.init();
    c.run();

    std::cout << "Count after " << c.task->max_iterations << " iterations --" << std::endl;
    std::cout << "    even: " << ctx.state.even << std::endl;
    std::cout << "     odd: " << ctx.state.odd << std::endl;

    std::fstream f { "hydra-history.csv" , std::ios::out };
    f << "even\todd\n";
    for (uint64_t i = 0; i < ctx.history.size(); i++) {
        f << ctx.history[i].even << "\t" << ctx.history[i].odd << "\n";
    }
    f.flush();

    return 0;
}

