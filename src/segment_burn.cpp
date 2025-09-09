#include <gmp.h>
#include <gmpxx.h>
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "segment.h"
#include "communicate.h"
#include "metrics.h"

void setup_vars(data_t* data) {
    segment_t* seg = data->segment;
    int rank = seg->world_rank;
    seg->is_base_segment = rank == 0;
    seg->is_top_segment = rank == seg->world_size-1;

    vars_t* vars = data->vars;
    vars->update = (mpz_ptr) malloc (sizeof(mpz_t));
    vars->p3 = {};
    vars->tmp = {};
    vars->stored = {};

    std::vector<std::vector<uint64_t>> sizes_ramp = data->config->block_sizes_funnel;
    std::vector<std::vector<uint64_t>> sizes_plat = data->config->block_sizes_chain;
    int min_plat_index = sizes_ramp.size();
    uint64_t offset = 0;
    for (int i = 0; i < rank; i++) {
        std::vector<uint64_t> list = i < min_plat_index ? sizes_ramp[i] : sizes_plat[i%min_plat_index];
        for (size_t j = 0; j < list.size(); j++) {
            offset += (uint64_t)1<<list[j];
        }
    }
    auto list = rank < min_plat_index ? sizes_ramp[rank] : sizes_plat[rank%min_plat_index];
    for (size_t j = 0; j < list.size(); j++) {
        // segments keep blocks in reversed order
        // int i = list.size() - j - 1;
        data->vars->global_offset.insert(data->vars->global_offset.begin(), offset);
        data->vars->block_size.insert(data->vars->block_size.begin(), list[j]);
        offset += (uint64_t)1<<list[j];
    }

    uint64_t max_size = vars->block_size[0];
    mpz_t r; mpz_init_set_ui(r, 3);
    for (uint64_t i = 0; i <= max_size; i++) {
        // 3^(2^0) = 3^1 = 3 is the first element of p3
        mpz_ptr next = (mpz_ptr) malloc (sizeof(mpz_t));
        mpz_init_set(next, r);
        vars->p3.push_back(next);
        // skip the last squaring
        if (i < max_size) {
            mpz_mul(r, r, r);
        }
    }
    mpz_clear(r);

    const int s = vars->block_size.size();
    for (int i = 0; i < s; i++) {
        mpz_ptr a = (mpz_ptr) malloc (sizeof(mpz_t));
        mpz_init(a);
        vars->tmp.push_back(a);
        mpz_ptr b = (mpz_ptr) malloc (sizeof(mpz_t));
        mpz_init(b);
        vars->stored.push_back(b);
    }

    if (seg->is_base_segment) {
        mpz_set_ui(vars->stored[vars->stored.size()-1], data->problem->initial);
    }
    mpz_ptr stored = vars->stored[vars->stored.size()-1];
    gmp_printf("rank %d init to %Zd\n", data->segment->world_rank, vars->stored[vars->stored.size()-1]);
    gmp_printf("rank %d aka init to %Zd\n", data->segment->world_rank, stored);
}

data_t* segment_init(problem_t* problem, config_t* config, segment_t* segment) {
    metrics_t* metrics = (metrics_t*) malloc (sizeof(metrics_t));
    data_t* data = (data_t*) malloc (sizeof(data_t));
    vars_t* vars = (vars_t*) malloc (sizeof(vars_t));
    init_metrics(metrics);
    timer_start(metrics, initializing);
    *data = {
        .problem = problem,
        .config = config,
        .segment = segment,
        .vars = vars,
        .metrics = metrics,
    };
    setup_vars(data);
    timer_stop(metrics, initializing);
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
int segment_burn(data_t* data, int64_t max_iterations) {
    uint64_t e = nearest2pow(static_cast<uint64_t>(max_iterations)); // log iterations
    uint64_t l = data->vars->block_size[0]; // log size
    // iterations can't exceed size because that causes problems
    // either in validity or in the memory architecture
    if (e >= l) {
        e = l;
    } else {
        // TODO: handle small steps
    }
    segment_t* segment = data->segment;
    vars_t* vars = data->vars;

    // TODO: this is also part of the left-truncation condition.
    // When the result leaves the segment's light cone, it should
    // be able to simply disappear.
    // Currently, just a crude approximation so that we use finite space.
    bool dont_communicate_left = segment->is_top_segment;

    mpz_ptr update = vars->update;
    // TODO: again, rop and add parameters could be merged
    mpz_t output; mpz_init(output);
    // Note that this timer is paused at the leaf cases.
    timer_start(data->metrics, grinding_chain);
    recursive_burn(data, output, update, e, 0);
    timer_stop(data->metrics, grinding_chain);

    // lower node sends first (to cleanup memory for lower levels (!?))
    if (!dont_communicate_left) {
        // gmp_printf("%d      sent left: %d bits\n", segment->world_rank, mpz_sizeinbase(output, 2));
        sendLeft(data, output);
    } else {
        // assert(mpz_sgn(output) == 0);
        // mpz_set(update, output);
        mpz_mul_2exp(update, output, 1<<l);
    }
    mpz_clear(output);

    if (!dont_communicate_left) {
        receiveLeft(data, update);
        // gmp_printf("%d  received left: %d bits\n", segment->world_rank, mpz_sizeinbase(output, 2));
    }

    // Problem... why is this now happening _before_ the computation,
    // while the recursive-burn's addition happens after?
    mpz_add(data->vars->stored[0], data->vars->stored[0], update);
    mpz_set_ui(update, 0);

    // compensating for small shifts is not necessary as long
    // as they remain in sync
    // however right-shifts have to be adjusted for being smaller?

    return 1<<e;
}

void segment_finalize(data_t* data) {
    // If (when) the segment didn't send its update, it needs
    // to re-inflate it and add it onto itself.
    const uint64_t l = data->vars->block_size[0]; // log size
    mpz_ptr update = data->vars->update;
    mpz_ptr stored = data->vars->stored[0];
    if (!data->segment->is_top_segment) {
        mpz_mul_2exp(update, update, 1<<l);
    }
    mpz_add(stored, stored, update);
}

// Funnel until next block, denoted by index i
// Updates x, representing the entire right side of the integer.
void funnel_until(data_t* data, mpz_t x, uint64_t e, int i) {
    const uint64_t end_size = data->vars->block_size[i];
    assert(e >= end_size);
    std::vector<mpz_ptr> p3 = data->vars->p3;
    if (e == end_size) {
        // x *= p3t
        // return top(x) + recv_carry(tail(x))
        mpz_mul(x, x, p3[e]);
        mpz_t tmp2; mpz_init(tmp2);
        mpz_fdiv_r_2exp(tmp2, x, 1<<e);
        mpz_fdiv_q_2exp(x, x, 1<<e);
        mpz_t res; mpz_init(res);
        recursive_burn(data, res, tmp2, e, i);
        mpz_clear(tmp2);
        // TODO: ideally no allocate or deallocate of mpz_t
        // Technically, we could pass the same mpz into both...
        // they're never needed at the same time
        mpz_add(x, x, res);
        mpz_clear(res);
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
            mpz_mul(x, x, p3[e-1]);
            mpz_add(x, x, next);
        }
        mpz_clear(next);
    }
    // Return is handled by updating x.
}

// Recursive burn moves depth-first from left to right,
//  possibly with some branching. Each i represents one block
//  of memory, regardless of step size. Steps size adapts (by
//  halving) so that it doesn't overflow the memory to the right.
// Should work with arbitrary memory blocks, including:
//  1) several same-sized blocks
//  2) non-decreasing sizes, adjusting steps as needed (not necssarily coalescing optimally)
//  3) no blocks at all, simply passing data through [suspect]
// Calling convention: `add` is passed undercarry, then the carry is
//  returned through `rop`.
// Note that carry must be added after the current block is updated.
// This method is a serial approximation of the parallel concept
//  of "multiply each block, then add the overflows to the left and right":
// 1) Accept undercarry parameter
// 2) Perform own computation
// 3) Add undercarry
// 4) Compute and return overcarry
// Some carries inevitably have to be stored, but it may be possible to
// delay the storage of the big ones.
void recursive_burn(data_t* data, mpz_t rop, mpz_t add, uint64_t e, int i) {
    segment_t* segment = data->segment;
    std::vector<uint64_t> blocks = data->vars->block_size;
    uint64_t l = blocks[i]; // log size of input/self
    mpz_ptr stored = data->vars->stored[i];
    mpz_ptr tmp = data->vars->tmp[i];
    std::vector<mpz_ptr> p3 = data->vars->p3;
    if (i == static_cast<int>(blocks.size()) - 1) {
        // Therefore we have the right size to pass to the next node.
        uint64_t t = 1<<e;
        // TODO: store this
        mpz_t ret; mpz_init(ret);
        if (segment->is_base_segment) {
            // Time to iterate basecase.
            // This function handles everything it needs already.
            // TODO: basecase_burn handles way too much, why, how?
            timer_stop(data->metrics, grinding_chain);
            timer_start(data->metrics, grinding_basecase);
            basecase_burn(data, ret, add, e, i);
            mpz_set(rop, ret);
            mpz_clear(ret);
            timer_stop(data->metrics, grinding_basecase);
            timer_start(data->metrics, grinding_chain);
            return;
        } else {
            // Otherwise continue passing data forth.
            mpz_mul(stored, stored, p3[e]);
            mpz_fdiv_r_2exp(tmp, stored, t);
            mpz_fdiv_q_2exp(stored, stored, t);
            timer_stop(data->metrics, grinding_chain);
            receiveRight(data, ret);
            // gmp_printf("%d  received right: %d bits\n", segment->world_rank, mpz_sizeinbase(ret, 2));
            sendRight(data, tmp);
            // gmp_printf("%d      sent right: %d bits\n", segment->world_rank, mpz_sizeinbase(tmp, 2));
            counter_count(data->metrics, messages_received_right);
            if (mpz_sgn(ret) != 0) {
                counter_count(data->metrics, messages_received_right_nonempty);
            }
            timer_start(data->metrics, grinding_chain);
            mpz_add(stored, stored, ret);
        }
        mpz_clear(ret);
    } else {
        funnel_until(data, stored, e, i+1);
    }
    if (segment->world_rank == 1) {
        
    }
    mpz_add(stored, stored, add);
    mpz_fdiv_q_2exp(tmp, stored, 1<<l);
    mpz_fdiv_r_2exp(stored, stored, 1<<l);
    mpz_set(rop, tmp); // TODO: work with rop as scratch?
}

void basecase_burn(data_t* data, mpz_t rop, mpz_t add, uint64_t e, int i) {
    // It seems like the vector contains the mpz structs themselves.
    // Thus dereferencing it takes a copy of size and limb pointer,
    // which promptly get overridden when an operation is made, and the
    // old struct is in invalid memory or something.
    mpz_ptr stored = data->vars->stored[i];
    mpz_ptr tmp = data->vars->tmp[i];
    uint64_t l = data->vars->block_size[i];
    uint64_t t = 1<<e;

    for (uint64_t i = 0; i < t; i++) {
        mpz_fdiv_q_2exp(tmp, stored, 1);
        mpz_add(stored, stored, tmp);
    }

    mpz_add(stored, stored, add);
    mpz_fdiv_q_2exp(rop, stored, 1<<l);
    mpz_fdiv_r_2exp(stored, stored, 1<<l);
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

