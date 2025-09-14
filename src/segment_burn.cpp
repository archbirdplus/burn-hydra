#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "segment.h"
#include "communicate.h"
#include "metrics.h"

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
void recursive_burn(data_t*, fmpz_t, fmpz_t, uint64_t, int);
void funnel_until(data_t*, fmpz_t, uint64_t, int);
void basecase_burn(data_t*, fmpz_t, fmpz_t, uint64_t, int);

// Returns number of iterations actually completed.
int segment_burn(data_t* data, int64_t max_iterations) {
    uint64_t e = nearest2pow(static_cast<uint64_t>(max_iterations)); // log iterations
    uint64_t l = data->vars->block_size[0]; // log size
    // iterations can't exceed size because that causes problems
    // either in validity or in the memory architecture
    if (e >= l) {
        // TODO: this is the only reason it's not trying to do 2^1048576 size
        // steps. e is a number of iterations, not its logarithm
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

    fmpz* update = &vars->update;
    // TODO: again, rop and add parameters could be merged
    fmpz_t output; fmpz_init(output);
    // Note that this timer is paused at the leaf cases.
    timer_start(data->metrics, grinding_chain);
    recursive_burn(data, output, update, e, 0);
    timer_stop(data->metrics, grinding_chain);

    // lower node sends first (to cleanup memory for lower levels (!?))
    if (!dont_communicate_left) {
        // gmp_printf("%d      sent left: %d bits\n", segment->world_rank, fmpz_sizeinbase(output, 2));
        sendLeft(data, output);
    } else {
        fmpz_mul_2exp(update, output, (uint64_t)1<<l);
    }
    fmpz_clear(output);

    if (!dont_communicate_left) {
        receiveLeft(data, update);
        // gmp_printf("%d  received left: %d bits\n", segment->world_rank, fmpz_sizeinbase(output, 2));
    }

    // Problem... why is this now happening _before_ the computation,
    // while the recursive-burn's addition happens after?
    fmpz_add(&data->vars->stored[0], &data->vars->stored[0], update);
    fmpz_set_ui(update, 0);

    // compensating for small shifts is not necessary as long
    // as they remain in sync
    // however right-shifts have to be adjusted for being smaller?

    return (uint64_t)1<<e;
}

void segment_finalize(data_t* data) {
    // If (when) the segment didn't send its update, it needs
    // to re-inflate it and add it onto itself.
    const uint64_t l = data->vars->block_size[0]; // log size
    fmpz* update = &data->vars->update;
    fmpz* stored = &data->vars->stored[0];
    if (!data->segment->is_top_segment) {
        fmpz_mul_2exp(update, update, (uint64_t)1<<l);
    }
    fmpz_add(stored, stored, update);
}

// Funnel until next block, denoted by index i
// Updates x, representing the entire right side of the integer.
void funnel_until(data_t* data, fmpz_t x, uint64_t e, int i) {
    const uint64_t end_size = data->vars->block_size[i];
    assert(e >= end_size);
    std::vector<fmpz> p3 = data->vars->p3;
    if (e == end_size) {
        // x *= p3t
        // return top(x) + recv_carry(tail(x))
        fmpz_mul(x, x, &p3[e]);
        fmpz_t tmp2; fmpz_init(tmp2);
        fmpz_fdiv_r_2exp(tmp2, x, (uint64_t)1<<e);
        fmpz_fdiv_q_2exp(x, x, (uint64_t)1<<e);
        fmpz_t res; fmpz_init(res);
        recursive_burn(data, res, tmp2, e, i);
        fmpz_clear(tmp2);
        // TODO: ideally no allocate or deallocate of mpz_t
        // Technically, we could pass the same mpz into both...
        // they're never needed at the same time
        fmpz_add(x, x, res);
        fmpz_clear(res);
    } else {
        // e > end_size
        // TODO: preallocate this
        fmpz_t next; fmpz_init(next);
        // high, one low per stack
        // high is n/2, low is actually n*1.6
        uint64_t t = (uint64_t)1<<(e-1);
        fmpz_set(next, x);
        for (int j = 0; j < 2; j++) {
            fmpz_fdiv_r_2exp(next, x, t);
            fmpz_fdiv_q_2exp(x, x, t);
            // most of the size thus remains in x
            // I guess they are both in cache though
            funnel_until(data, next, e-1, i);
            // x re-inflates after the longer process
            fmpz_mul(x, x, &p3[e-1]);
            fmpz_add(x, x, next);
        }
        fmpz_clear(next);
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
void recursive_burn(data_t* data, fmpz_t rop, fmpz_t add, uint64_t e, int i) {
    segment_t* segment = data->segment;
    std::vector<uint64_t> blocks = data->vars->block_size;
    uint64_t l = blocks[i]; // log size of input/self
    fmpz* stored = &data->vars->stored[i];
    fmpz* tmp = &data->vars->tmp[i];
    std::vector<fmpz> p3 = data->vars->p3;
    if (i == static_cast<int>(blocks.size()) - 1) {
        // Therefore we have the right size to pass to the next node.
        uint64_t t = (uint64_t)1<<e;
        // TODO: store this
        fmpz_t ret; fmpz_init(ret);
        if (segment->is_base_segment) {
            // Time to iterate basecase.
            // This function handles everything it needs already.
            // TODO: basecase_burn handles way too much, why, how?
            timer_stop(data->metrics, grinding_chain);
            timer_start(data->metrics, grinding_basecase);
            basecase_burn(data, ret, add, e, i);
            fmpz_set(rop, ret);
            fmpz_clear(ret);
            timer_stop(data->metrics, grinding_basecase);
            timer_start(data->metrics, grinding_chain);
            return;
        } else {
            // Otherwise continue passing data forth.
            fmpz_mul(stored, stored, &p3[e]);
            fmpz_fdiv_r_2exp(tmp, stored, t);
            fmpz_fdiv_q_2exp(stored, stored, t);
            timer_stop(data->metrics, grinding_chain);
            receiveRight(data, ret);
            // gmp_printf("%d  received right: %d bits\n", segment->world_rank, fmpz_sizeinbase(ret, 2));
            sendRight(data, tmp);
            // gmp_printf("%d      sent right: %d bits\n", segment->world_rank, fmpz_sizeinbase(tmp, 2));
            counter_count(data->metrics, messages_received_right);
            if (fmpz_sgn(ret) != 0) {
                counter_count(data->metrics, messages_received_right_nonempty);
            }
            timer_start(data->metrics, grinding_chain);
            fmpz_add(stored, stored, ret);
        }
        fmpz_clear(ret);
    } else {
        funnel_until(data, stored, e, i+1);
    }
    if (segment->world_rank == 1) {
        
    }
    fmpz_add(stored, stored, add);
    fmpz_fdiv_q_2exp(tmp, stored, (uint64_t)1<<l);
    fmpz_fdiv_r_2exp(stored, stored, (uint64_t)1<<l);
    fmpz_set(rop, tmp); // TODO: work with rop as scratch?
    // note: that might reduce ram infighting with multiple threads
}

void basecase_burn(data_t* data, fmpz_t rop, fmpz_t add, uint64_t e, int block) {
    fmpz* fstored = &data->vars->stored[block];
    fmpz* ftmp = &data->vars->tmp[block];
    uint64_t l = data->vars->block_size[block];
    basecase_table_t* table = data->vars->basecase_table;
    uint64_t bits = data->vars->table_bits;
    uint64_t base = (uint64_t)1<<bits;
    uint64_t mask = base-1;
    uint64_t p3 = data->vars->p3base;
    uint64_t t = (uint64_t)1<<e;

    _fmpz_promote_val(fstored);
    _fmpz_promote_val(ftmp);
    mpz_ptr stored = COEFF_TO_PTR(*fstored);
    mpz_ptr tmp = COEFF_TO_PTR(*ftmp);

    uint64_t i = 0;
    for (; i < t - bits; i += bits) {
        uint64_t index = mpz_get_ui(stored) & mask;
        mpz_fdiv_q_2exp(tmp, stored, bits);
        uint64_t mem = static_cast<uint64_t>(table[index]);
        mpz_mul_ui(stored, tmp, p3);
        mpz_add_ui(stored, stored, mem);
    }
    for (; i < t; i += 1) {
        mpz_fdiv_q_2exp(tmp, stored, 1);
        mpz_add(stored, stored, tmp);
    }

    fmpz_add(fstored, fstored, add);
    fmpz_fdiv_q_2exp(rop, fstored, (uint64_t)1<<l);
    fmpz_fdiv_r_2exp(fstored, fstored, (uint64_t)1<<l);
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

