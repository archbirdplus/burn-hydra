#pragma once

#include "state.h"

struct context_MPI {
    
void step_MPI(Context* ctx, kernel_MPI_t* kMPI<kernel) {
    kMPI::Inner::run<exportL_MPI, exportR_MPI>(ctx, kMPI.inner_ctx, kMPI);
}
}

void loop1_fmpz_syncR<syncR2>(Context* ctx, l1contex<fmpz>* l1, int64_t n, uint64_t step_size) {
    // multiply by r, shift carry down
    if (n == 0) {
        syncR2(l1.undercarry.front());
    } else {
        // it's already in the undercarry vector
    }
}

void loop1_fmpz_syncL<syncL2>(Context* ctx, l1contex<fmpz>* l1, int64_t n) {
    // add carry
    if (n == l1.blocks_count-1) {
        // presumably requires L2 context though
        syncL2(l1.overcarry.back());
    } else {
        // it's already in the undercarry vector
    }
}

void loop1_fmpz<syncL1, syncR1, syncL2, syncR2>(Context* ctx, l1contex<fmpz>* l1, int64_t n) {
    if (n < 0) return;
    int64_t pow = l1.power[n];
    int64_t step_size = l1.next_step[n];
    for (int64_t i = 0; i < pow; i++) {
        loop1_fmpz(ctx, l1, n-1);
        loop1_fmpz_syncR(ctx, l1, n, step_size);
        if (n == 0) {
            syncR2(ctx, l1.stored.
        }
    }
    loop1_fmpz_syncL(ctx, l1, n);
}

void generic_basecase<

void binary_recursion<Kernel>(Kernel* kernel, int64_t n) {
    int64_t pow = Kernel::power(kernel, n);
    for (int64_t i = 0; i < pow; i++) {
        binary_recursion<Kernel>(kernel, n-1);
        Kernel::syncR(kernel, n);
    }
    Kernel::syncL(kernel, n);
}

void generic_exportR_fmpz<U>(consistent_kernel<fmpz, U>* kernel, fmpz x, int64_t n) {
    if (n > 0) {
        // Default to old style behavior.
        // Alternatively, have the output always be to a carry buffer,
        // leaving this as a no-op.
        // In that case, MPI/threads level kernels can write directly
        // into fmpz-level buffers?
        // This should be a flint no-op if writes always happen to
        // undercarry buffer and exports come from them.
        fmpz_set_ui(&kernel->undercarry[n], x);
    } else if (U != void) {
        // fmpz not copied, and upper kernel receives the same sort of reference and
        // the same possibility for optimization.
        // Thus the api doesn't use extra buffer copies but...
        // The upper level will still need to copy if MPI, or if single-threaded
        // unless it somehow links the integers together (which would require
        //  some invasion of concerns (and is it possbile if the vector contains
        //  fmpz and not fmpz_t?).

        // (or K::U::exportR)
        // give up and just let this be done for each kernel individually,
        // it's just one loop anyways; just generic the upper kernel's import/export
        // for boundaries; implement sync/import/receive style internally for now
        U::exportR(kernel->upper_kernel, x, kernel->upper_position);
        // Basecase shouldn't be exporting at all.
    }
}

using A = consistent_kernel<B, void>
using B = consistent_kernel<fmpz, A>
consistent_kernel<consistent_kernel<fmpz>, >

// Where T may be an fmpz or another consistent kernel.
struct l1context<T> {
    Context* context;

    U* upper_kernel;
    uint64_t upper_position;

    // block powers?

    vec<T> stored;
    vec<fmpz> undercarry;
    vec<fmpz> overcarry;
}

template<ramp_fn, basecase_fn>
typedef struct kernel {
    using ramp = ramp;
    using basecase = basecase_fn;

    // From top segments to basecase as leaf.
    void binary_recursion(Context* ctx, int64_t segment) {
        if (segment < 0) {
            basecase::basecase_step(ctx);
            return;
        }
        if (segment >= ctx.task.world_rank-1) {
            bool second_pass
            uint64_t this_size = ctx->task.block_sizes[segment].back();
            uint64_t next_size = ctx->task.block_sizes[segment-1].front();

            // Compute carry from zero-initialized update.
            binary_recursion(ctx, segment-1);
            ramp::syncR(ctx, segment);

            if (ctx.task.block_sizes[segment] > 
                binary_recursion(ctx, segment-1);
                ramp::syncR(ctx, segment);
            }
        }
        // chop upwards update and load down
        ramp::syncL(ctx, segment);
    }

    void run(Context* ctx) {
        while(true) {
            binary_recursion(ctx, ctx.task.block_sizes.size()-1);
        }
    }
}

