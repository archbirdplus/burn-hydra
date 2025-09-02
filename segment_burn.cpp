#include <gmp.h>
#include <gmpxx.h>
#include <cassert>
#include <cstdint>
#include <vector>

#include "communicate.h"

typedef struct vars {
    mpz_class update;
    std::vector<mpz_class> p3;
    std::vector<mpz_class> tmp;
    std::vector<mpz_class> stored;
    std::vector<uint64_t> block_size; // from left to right, including input (stored) and output (not stored) sizes
} vars_t;

typedef struct data {
    problem_t* problem;
    config_t* config;
    segment_t* segment;
    vars_t* vars;
} data_t;

void setup_vars(data_t* data) {
    segment_t* seg = data->segment;
    seg->is_base_segment = seg->world_rank == 0;
    seg->is_top_segment = seg->world_rank == seg->world_size-1;

    vars_t* vars = data->vars;
    vars->update = mpz_class(0); // TODO: coalesce this into another tmp
    vars->p3 = {};
    vars->tmp = {};
    vars->stored = {};
    if (seg->is_base_segment) {
        vars->block_size = {10,8};
    } else {
        vars->block_size = {10}; // TODO: optimize
    }

    uint64_t max_size = vars->block_size[0];
    mpz_t r; mpz_init_set_ui(r, 3);
    for (uint64_t i = 1; i <= max_size; i++) {
        mpz_t next; mpz_init_set(next, r); // 3^(2^0) = 3^1 = 3 as the first
        vars->p3.push_back(mpz_class(r));
        mpz_mul(r, r, r); // could be skipped at end
        vars->tmp.push_back(mpz_class(0));
        vars->stored.push_back(mpz_class(0));
    }
    mpz_clear(r);
}

void* segment_init(problem_t* problem, config_t* config, segment_t* segment) {
    data_t* data = (data_t*)malloc(sizeof(data_t));
    vars_t* vars = (vars_t*)malloc(sizeof(vars_t));
    *data = {
        .problem = problem,
        .config = config,
        .segment = segment,
        .vars = vars,
    };
    setup_vars(data);
    return data;
}

// Largest power of 2 up to and including x.
// https://stackoverflow.com/questions/4398711/round-to-the-nearest-power-of-two#4398845
uint64_t nearest2pow(uint64_t x) {
    uint64_t v = x;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    v >>= 1;
    return v;
}

int segment_burn(data_t*, int);
void recursive_burn(data_t*, mpz_t, mpz_t, uint64_t, int);
void funnel_until(data_t*, mpz_t, uint64_t, int);
void basecase_burn(data_t*, mpz_t, mpz_t, uint64_t, int);

// Returns number of iterations actually completed.
int segment_burn(void* v, int max_iterations) {
    data_t* data = (data_t*)v;
    uint64_t e = nearest2pow(static_cast<uint64_t>(max_iterations)); // log iterations
    uint64_t l = data->vars->block_size[0]; // log size
    // iterations can't exceed size because that causes problems
    // either in validity or in the memory architecture
    if (e >= l) {
        e = l;
    } else {
        // An interesting situtation, we might accidentally hit the basecase.
        // But that is not necessarily invalidating.
    }
    segment_t* segment = data->segment;
    vars_t* vars = data->vars;

    // TODO: this is also part of the left-truncation condition.
    // When the result leaves the segment's light cone, it should
    // be able to simply disappear.
    // Currently, just a crude approximation so that we use finite space.
    bool dont_communicate_left = segment->is_top_segment;

    mpz_class update = vars->update;
    mpz_t output; mpz_init(output);
    // TODO: again, rop and add parameters could be merged
    recursive_burn(data, output, update.get_mpz_t(), e, 0);

    // lower node sends first (to cleanup memory for lower levels (!?))
    if (!dont_communicate_left) {
        sendLeft(segment, output);
    } else {
        assert(mpz_sgn(output) == 0);
        mpz_set(update.get_mpz_t(), output);
    }
    mpz_clear(output); // TODO: again, remove alloc

    if (!dont_communicate_left) {
        receiveLeft(segment, update.get_mpz_t());
    }

    // compensating for small shifts is not necessary as long
    // as they remain in sync
    // however right-shifts have to be adjusted for being smaller?
    // TODO: let funnel soak up and coalesce small shifts

    return 1<<e;
}

// Funnel until next block, denoted by index i
// Updates x, representing the entire right side of the integer.
void funnel_until(data_t* data, mpz_t x, uint64_t e, int i) {
    const uint64_t end_size = data->vars->block_size[i];
    assert(e >= end_size);
    std::vector<mpz_class> p3 = data->vars->p3;
    if (e == end_size) {
        // end of funnel, go to next mem block
        // it will return the integer to pass back up
        // TODO: this bit of the integer hasn't been updated yet,
        // and the recursive burn still has to return its carry
        // calling convention may differ
        // x *= p3t
        // return top(x) + recv_carry(tail(x))
        mpz_mul(x, x, p3[e].get_mpz_t());
        mpz_t tmp2; mpz_init(tmp2);
        mpz_fdiv_r_2exp(tmp2, x, 1<<e);
        mpz_fdiv_q_2exp(x, x, 1<<e);
        mpz_t res; mpz_init(res);
        recursive_burn(data, res, tmp2, e, i);
        mpz_clear(tmp2);
        // TODO: ideally no allocate or deallocate of mpz_t
        // Technically, we could pass the same mpz into both...
        // they're never needed at the same time
        mpz_clear(res);
        mpz_add(x, x, res);
    } else {
        // e > end_size
        // TODO: preallocate this
        mpz_t next; mpz_init(next);
        // high, one low per stack
        // high is n/2, low is actually n*1.6
        uint64_t t = 1<<(e-1);
        mpz_set(next, x);
        for (int j = 0; j < 2; j++) {
            mpz_fdiv_r_2exp(next, x, t);
            mpz_fdiv_q_2exp(x, x, t);
            // most of the size thus remains in x
            // I guess they are both in cache though
            funnel_until(data, next, e-1, i);
            // x re-inflates after the longer process
            mpz_mul(x, x, p3[t].get_mpz_t());
            mpz_add(x, x, next);
        }
        mpz_clear(next);
    }
    // Return is handled by updating x.
}

// Recursive burn moves depth-first from left to right,
// possibly with some branching. Each i represents one _block
// of memory_, regardless of step size. Steps size adapts (by
// halving) so that it doesn't overflow the memory to the right.
// Should work with arbitrary memory blocks, including:
//  1) several same-sized ones, without fully recombining them
//  2) non-monotinic non-decreasing sizes, adjusting steps as needed (not necssarily coalescing optimally)
//  3) zero blocks, simply passing data through [maybe not]
// TODO: currently data is not properly separated? all comes from x
// Calling convention: x is passed undercarry, then the carry is returned
// Note that carry must be added after the current block is updated.
// 1) Accept undercarry parameter
// 2) Perform own computation
// 3) Add undercarry
// 4) Compute and return overcarry
// ... could we rearrange it to add first while cache is hot?
void recursive_burn(data_t* data, mpz_t rop, mpz_t add, uint64_t e, int i) {
    segment_t* segment = data->segment;
    std::vector<uint64_t> blocks = data->vars->block_size;
    uint64_t l = blocks[i]; // log size of input/self
    mpz_class stored = data->vars->stored[i];
    mpz_class tmp = data->vars->tmp[i];
    std::vector<mpz_class> p3 = data->vars->p3;
    // TODO: blocks_count
    // TODO: does the last "block" need to update? is it not an update itself?
    if (i == blocks.size() - 1) {
        // Therefore we have the right size to pass to the next node.
        uint64_t t = 1<<e;
        // mpz_mul(stored.get_mpz_t(), stored.get_mpz_t(), p3[e].get_mpz_t());
        stored = stored * p3[e];
        mpz_fdiv_r_2exp(tmp.get_mpz_t(), stored.get_mpz_t(), t);
        // TODO: store this
        mpz_t ret; mpz_init(ret);
        if (segment->is_base_segment) {
            // Time to iterate basecase.
            basecase_burn(data, ret, tmp.get_mpz_t(), e, i+1);
        } else {
            // Otherwise continue passing data forth.
            mpz_init(ret);
            // TODO: ideally recv/send the buffer than afterwards format it...
            // check perf though
            receiveRight(segment, ret);
            sendRight(segment, tmp.get_mpz_t());
        }
        mpz_add(stored.get_mpz_t(), stored.get_mpz_t(), ret);
    } else {
        funnel_until(data, stored.get_mpz_t(), e, i+1);
    }
    mpz_add(stored.get_mpz_t(), stored.get_mpz_t(), add);
    mpz_fdiv_q_2exp(tmp.get_mpz_t(), stored.get_mpz_t(), 1<<l);
    mpz_fdiv_r_2exp(stored.get_mpz_t(), stored.get_mpz_t(), 1<<l);
    // mpz_set(rop, tmp);
    mpz_set(rop, tmp.get_mpz_t()); // TODO: ???
}

void basecase_burn(data_t* data, mpz_t rop, mpz_t add, uint64_t e, int i) {
    mpz_class stored = data->vars->stored[i];
    mpz_class tmp = data->vars->tmp[i];
    uint64_t l = data->vars->block_size[i];
    mpz_add(stored.get_mpz_t(), stored.get_mpz_t(), add);
    uint64_t t = 1<<e;

    for (uint64_t i = 0; i < t; i++) {
        mpz_fdiv_q_2exp(tmp.get_mpz_t(), stored.get_mpz_t(), 1);
        mpz_add(stored.get_mpz_t(), stored.get_mpz_t(), tmp.get_mpz_t());
    }

    mpz_fdiv_q_2exp(rop, stored.get_mpz_t(), 1<<l);
    mpz_fdiv_r_2exp(stored.get_mpz_t(), stored.get_mpz_t(), 1<<l);
}

// treating as message-passing and slightly inefficient but instead
// eliminating the need for full addition -> simpler parallelization
// Or: the max shift can be reduced compared to hydra_fast:
// max shift to be the lower node's message size, rather than the sum
// This probably makes more sense with more nodes but...
// it performs 3/4 of the multiplication work for 1/2 of the results
// In general a shift of k on n gives k bits for k+n: k/(k+n) relative work
// But 1.6k+n memory, and k/n communications per node
// _everything_ can be time-efficient, since it's just the funnel:
// 1) internally compute with full integer
// 2) expose entire span to messages, unlike split chain-nodes
// 3) split up by nodes if needed by profiling?
// the rest optimized for space
// OR we could have it break on listed memory regions, more control

// TODO: formalize mathematical description (k=1,2) for documentation
// t = 1<<(e-1)
// tmp = x
// for i in {0,1}
    // high = tmp >> t
    // low = tmp % 2^t
    // tmp = (3^t)*high + burn_funnel(low, e)

// void burn_funnel(data_t* data, mpz_t x, uint64_t e, uint64_t l) {
    // high, one low per stack
    // high is n/2, low is actually n*1.6
    // t = 1<<(e-1)
    // low = x
    // for i in {0,1}
        // high = low >> t
        // low %= 2^t
        // low = (3^t)*high + burn_funnel(low, e-1)
    // return tmp
// }

