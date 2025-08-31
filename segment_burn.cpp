
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

void segment_burn(void* data, int max_iterations) {
    uint64_t e = nearest2pow(static_cast<uint64_t>(max_iterations)); // log iterations
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
    mpz_t result = recursive_burn(self, x, e, 1);
    mpz_fdiv_q_2exp(vars->update, result, l); // just the physical overflow
    mpz_fdiv_r_2exp(result, result, l);
    sendLeft(segment, update); // lower node sends first? (to cleanup memory for lower levels)
    // just... ignore and assert zeroes on the left edge
    receiveLeft(segment, update);
    // compensate for partial right shift
    // assume that the left node had the same iterations demanded
    mpz_mul_2exp(update, update, 1<<(l-e));
    mpz_add(result, result, update);
    result += update
}

// Funnel until next block, denoted by index i
// Updates x, representing the entire right side of the integer.
void funnel_until(data_t* data, mpz_t x, uint64_t e, int i) {
    const uint64_t end_size = data->vars->block_size[i];
    assert e >= end_size;
    vector<mpz_t> p3 = data->vars->p3;
    if (e == end_size) {
        // end of funnel, go to next mem block
        // it will return the integer to pass back up
        // TODO: this bit of the integer hasn't been updated yet,
        // and the recursive burn still has to return its carry
        // calling convention may differ
        // x *= p3t
        // return top(x) + recv_carry(tail(x))
        mpz_mul(x, x, p3[e]);
        mpz_t tmp2; mpz_init(tmp2);
        mpz_fdiv_r_2exp(tmp2, x, 1<<e);
        mpz_fdiv_q_2exp(x, x, 1<<e);
        mpz_t res;
        res = recursive_burn(data, tmp2, e, i);
        mpz_clear(tmp2);
        // TODO: this res is allocated in rec-burn
        // ideally neither would allocate or deallocate
        mpz_clear(res);
        mpz_add(x, x, res);
        return x;
    }
    // e > end_size
    // TODO: preallocate this
    mpz_t next; mpz_init(next);
    // high, one low per stack
    // high is n/2, low is actually n*1.6
    uint64_t t = 1<<(e-1)
    mpz_set(low, x);
    for (int j = 0; j < 2; j++) {
        mpz_fdiv_r_2exp(next, x, t);
        mpz_fdiv_q_2exp(x, x, t);
        // most of the size thus remains in x
        // I guess they are both in cache though
        funnel_until(data, next, e-1, i);
        // x re-inflates after the longer process
        mpz_mul(x, x, p3[t]);
        mpz_add(x, x, next);
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
// Calling convention: x is passed undercarry, then the carry is returned
// Note that carry must be added after the current block is updated.
// 1) Accept undercarry parameter
// 2) Perform own computation
// 3) Add undercarry
// 4) Compute and return overcarry
// ... could we rearrange it to add first while cache is hot?
mpz_t recursive_burn(data_t* data, mpz_t add, uint64_t e, int i) {
    segment_t* segment = data->segment;
    vector<uint64_t> blocks = data->vars->block_size
    uint64_t l = blocks[i]; // log size of input/self
    mpz_t stored = data->vars->stored[i];
    mpz_t tmp = data->vars->tmp[i];
    vector<mpz_t> p3 = data->vars->p3;
    if (i == blocks.length - 1) {
        // Therefore we have the right size to pass to the next node.
        uint64_t t = 1<<e;
        mpz_mul(stored, stored, p3[e]);
        mpz_fdiv_r_2exp(tmp, stored, t);
        // TODO: store this
        mpz_t ret;
        if (segment->is_final) {
            // Time to iterate basecase.
            ret = basecase_burn(tmp, e);
        } else {
            // Otherwise continue passing data forth.
            mpz_init(ret);
            // TODO: ideally recv/send the buffer than afterwards format it...
            // check perf though
            receiveRight(segment, ret);
            sendRight(segment, tmp);
        }
        mpz_add(stored, stored, ret);
    } else {
        funnel_until(data, stored, e, i+1);
    }
    mpz_add(stored, stored, add);
    mpz_fdiv_q_2exp(tmp, stored, 1<<l);
    mpz_fdiv_r_2exp(stored, stored, 1<<l);
    return tmp;
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

void burn_funnel(data_t* data, mpz_t x, uint64_t e, uint64_t l) {
    // high, one low per stack
    // high is n/2, low is actually n*1.6
    // t = 1<<(e-1)
    // low = x
    // for i in {0,1}
        // high = low >> t
        // low %= 2^t
        // low = (3^t)*high + burn_funnel(low, e-1)
    // return tmp
}

