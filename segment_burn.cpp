
#include <cstdint>

#include "communicate.h"

typedef struct vars {
    update;
    vector<mpz_t> p3t;
    vector<mpz_t> tmp;
    vector<uint64_t> block_size; // from left to right, including input and output sizes
} vars_t;

typdef struct data {
    problem_t* problem;
    config_t* config;
    segment_t* self;
} data_t;

void* segment_init(problem_t* problem, config_t* config, segment_t* self) {
    data = {
        .problem = problem,
        .config = config,
        .self = self,
    };
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

void segment_burn(void* data, max_iterations) {
    uint64_t e = nearest2pow(max_iterations); // log iterations
    uint64_t l = self.start_size; // log size
    // iterations can't exceed size because that causes problems
    // either in validity or in the memory architecture
    if (e >= l) {
        e = l;
    } else {
        // An interesting situtation, we might accidentally hit the basecase.
        // But that is not necessarily invalidating.
    }
    vars_t* vars = data->vars;
    segment_t* segment = data->segment;
    mpz_t update = vars->update;
    mpz_t result = select_burn(self, x, e, l);
    mpz_fdiv_q_2exp(vars->update, result, l); // just the physical overflow
    mpz_fdiv_r_2exp(result, result, l);
    sendLeft(segment, update); // lower node sends first? (to cleanup memory for lower levels)
    receiveLeft(segment, update);
    // compensate for partial right shift
    // assume that the left node had the same iterations demanded
    mpz_mul_2exp(update, update, 1<<(l-e));
    mpz_add(result, result, update);
    result += update
}

// Selects the appropriate implementing method,
// which themselves handle messages to lower nodes.
void select_burn(data_t* data, mpz_t x, uint64_t e, uint64_t l) {
    if e > self.end_size {
        // turn one big-step into smaller messages
        // handles everything down to the basecase (end_size=0)
        burn_funnel(data, x, e, l);
        return;
    }
    if e == self.end_size {
        // a single big-step operation
        burn_chain(data, x, e, l);
        return;
    }
}

// Recursive burn moves depth-first from left to right,
// possibly with some branching. Each i represents one _block
// of memory_, regardless of step size. Steps size adapts (by
// halving) so that it doesn't overflow the memory to the right.
// Should work with arbitrary memory blocks, including:
//  1) several same-sized ones, without fully recombining them
//  2) non-monotinic non-decreasing sizes, adjusting steps as needed (not necssarily coalescing optimally)
//  3) zero blocks, simply passing data through
// TODO: currently data is not properly separated? all comes from x
void recursive_burn(data_t* data, mpz_t x, uint64_t e, uint64_t i) {
    segment_t* segment = data->segment;
    vector<uint64_t> tmp = data->vars->block_size
    vector<uint64_t> blocks = data->vars->block_size
    uint64_t input = blocks[i];
    uint64_t output = blocks[i+1];
    if (i == blocks.length - 1) {
        // Therefore we have the right size to pass to the next node.
        if (segment->final) {
            // Time to iterate basecase.
            basecase_burn(x, e);
        } else {
            // Otherwise continue passing data forth.
            receiveRight(segment, tmp[i]);
            sendRight(segment, x);
            mpz_swap(x, tmp[i]);
            return x;
        }
    }
    int repetitions = 1<<(input-output);
    // how about only handle 1, 2 repetitions for now
    assert repetitions < 3
    if (repetitions == 1) {
        // TODO: check e
        mpz_mul(x, x, p3t[e]);
        uint64_t t = 1<<e;
        mpz_fdiv_q_2exp(z, x, t);
        mpz_fdiv_r_2exp(x, x, t);
        mpz_fdiv_q_2exp(w, x, t);
        mpz_fdiv_r_2exp(x, x, t);
        // this will probably simply pass the message along
        mpz_t wp = recursive_burn(z, e, i+1);
        // could return wp via z?
        mpz_add(x, x, wp); // eh, just leave the extra bit or two in there
        // TODO: store x
        // TODO: single "global" temporary variable?
        return w;
    } else if (repetitions == 2) {
        uint64_t t = 1<<(e-1);
        mpz_t tmp = vars->tmp[i];
        mpz_set(tmp, x);
        mpz_t high = vars->tmp[another];
        for (int i = 0; i < 2; i++) {
            mpz_fdiv_q_2exp(high, tmp, t);
            mpz_fdiv_r_2exp(tmp, tmp, t);
            mpz_mul(high, high, p3t[e-1]);
            // is "memory blocks" actually needed for this?
            // it's in the form of preallocated "stack" variables?
            // TODO: also consider the calling convention, maybe save a dup
            // tmp is not needed again here
            mpz_t res = recursive_burn(tmp, e-1, i+1);
            mpz_add(high, high, res);
            mpz_swap(high, tmp);
        }
        mpz_fdiv_q_2exp(tmp, tmp, 1<<(l-1));
        // TODO: store the rest...
        // can this storage be used over the temporary? or something
        // TODO: add stored to x input
        mpz_set(vars->store[i], tmp);
        return tmp;
    }
}

// treating as message-passing and slightly inefficient but instead
// eliminating the need for full addition -> simpler parallelization
// Or: the max shift can be reduced compared to hydra_fast:
// max shift to be the lower node's message size, rather than the sum
// This probably makes more sense with more nodes but...
// it performs 3/4 of the multiplication work for 1/2 of the results
// In general a shift of k on n gives k bits for k+n: k/(k+n)
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
    // low = tmp % t
    // tmp = (3^t)*high + burn_funnel(low, e)

void burn_funnel(data_t* data, mpz_t x, uint64_t e, uint64_t l) {
    // high, one low per stack
    // high is n/2, low is actually n*1.6
    // t = 1<<(e-1)
    // low = x
    // for i in {0,1}
        // high = low >> t
        // low %= t
        // low = (3^t)*high + burn_funnel(low, e-1)
    // return tmp
}

