#include "mpi.h"
#include "flint/ulong_extras.h"

#include <algorithm>
#include <limits>

#include "state.h"
#include "init.h"
#include "friendly_assert.h"
#include <cassert>
#include <stdexcept>

#include "kernels.h"

void ensure_MPI_init() {
    int flag;
    MPI_Initialized(&flag);
    if (!flag) {
        MPI_Init(NULL, NULL);
    }
}

Task::Task(const CollatzBuilder* setup) {
    bool error = false;
    bool* e = &error;
    checkpoint_interval = setup->checkpoint_interval;
    if(setup->collatz.has_value()) collatz = *setup->collatz;
    else friendly_concern(e, false, "Missing setup: collatz function");
    if(setup->initial.has_value()) initial = *setup->initial;
    else friendly_concern(e, false, "Missing setup: initial value");
    if(setup->max_iterations.has_value()) max_iterations = *setup->max_iterations;
    else friendly_concern(e, false, "Missing setup: max iterations");
    if(setup->max_iterations.has_value()) table_size = *setup->table_size;
    else friendly_concern(e, false, "Missing setup: table size");

    // This seems to work outside an MPI context; world_size would simply be 1.
    ensure_MPI_init();
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    thread_breaks = {};

    // TODO: automatically configure block sizes based on problem size, node count
    // TODO: check valid block sizes
    if(setup->block_sizes_ramp.has_value()) {
        vecvec<uint64_t> ramp = *setup->block_sizes_ramp;
        for (uint64_t i = 0; i < ramp.size(); i++) {
            thread_breaks.push_back({});
        }
        block_sizes = ramp;
        if((uint64_t)world_size > block_sizes.size()) {
            if(setup->block_sizes_plat.has_value()) {
                vecvec<uint64_t> plat = *setup->block_sizes_plat;
                int ramp_size = ramp.size();
                int plat_size = plat.size();
                for(int i = ramp_size; i < world_size; i++) {
                    thread_breaks.push_back({});
                    block_sizes.push_back(plat[(i - ramp_size) % plat_size]);
                }
            } else friendly_concern(e, false, "Missing setup: block sizes plateau");
        }
    } else friendly_concern(e, false, "Missing setup: block sizes");

    flint_threads = setup->flint_threads.value_or(1);
    scan_config = setup->scan_config;

    if(error) {
        throw std::runtime_error("Configuration is missing information");
    }
}

vec<fmpz> create_2exp_powers(uint64_t r, uint64_t n) {
    // Create vector of repeated squares r^2^k for k inclusive of [0, n]
    vec<fmpz> pR = {}; // begin with r^2^0 = r^1
    {
        fmpz next; fmpz_init_set_ui(&next, r);
        pR.push_back(r);
    }
    fmpz_t tmp; fmpz_init_set_ui(tmp, r);
    for(uint64_t i = 0; i < n; i++) {
        fmpz_mul(tmp, tmp, tmp);
        fmpz next; fmpz_init_set(&next, tmp);
        pR.push_back(next);
    }
    return pR;
}

Workspace::Workspace(const Task *task) {
    vec<uint64_t> my_blocks = task->block_sizes[task->world_rank];
    uint64_t largest_size = std::max_element(my_blocks.begin(), my_blocks.end())[0];
    pR = create_2exp_powers(task->collatz.r, largest_size);
    pM = create_2exp_powers(task->collatz.m, largest_size);
    // TODO: automatically configure table size
    basecase_table = create_basecase_table<uint64_t>(task->collatz, task->table_size);
}

Context::Context(const CollatzBuilder* setup) {
    metrics = std::unique_ptr<Metrics>(new Metrics(true));
    metrics->start_timer(active_time);
    metrics->start_timer(initializing);
    task = std::unique_ptr<Task>(new Task(setup));
    workspace = std::unique_ptr<Workspace>(new Workspace(task.get()));
    metrics->stop_timer(initializing);
}

void Context::run() {
    // TODO: aggressive specialization for:
    // base-2 r/m, thread/node counts, table size/type, scan styles


    // possibly something like this?
    // runner<kernel_ramp_consistent_m2exp, kernel_basecase_consistent_m2exp>(this).run();
    // runner<kernel_ramp_consistent, kernel_basecase_consistent>(this).run();
    std::cout << "Chose communicator: MPI" << std::endl;
    auto wrapper_mpi = Wrapper_MPI(this);
    auto wrapper_threads = Wrapper_threads(this, &wrapper_mpi);
    Wrapper* outer = &wrapper_threads;
    std::unique_ptr<Basecase_simple> basecase;
    vec<Burner*> burners = {};
    uint64_t thread_count = wrapper_threads.thread_count;
        for (uint64_t i = 0; i < thread_count; i++) {
        if (!task->scan_config || task->scan_config->scan_block_size == task->table_size) {
            std::cout << "Chose basecase: table" << std::endl;
            basecase = std::unique_ptr<Basecase_table>(new Basecase_table(this));
        } else if (task->collatz.m == (1 << n_flog(task->collatz.m, 2))) {
            std::cout << "Chose basecase: m2exp" << std::endl;
            basecase = std::unique_ptr<Basecase_m2exp>(new Basecase_m2exp(this));
        } else {
            std::cout << "Chose basecase: simple" << std::endl;
            basecase = std::unique_ptr<Basecase_simple>(new Basecase_simple(this));
        }
        Burner* burner;
        if ((1<<n_flog(task->collatz.m, 2)) == task->collatz.m) {
            std::cout << "Chose chain: m2exp" << std::endl;
            burner = new Burner_m2exp(this, outer, std::move(basecase));
        } else {
            std::cout << "Chose chain: simple" << std::endl;
            burner = new Burner_simple(this, outer, std::move(basecase));
        }
        burners.push_back(burner);
    }

    flint_set_num_threads(task->flint_threads);
    uint64_t destination = this->task->max_iterations;
    wrapper_mpi.run_until(destination);

    std::cout << "Finished: rank " << task->world_rank << std::endl;
    metrics->stop_timer(active_time);
    metrics->dump_with_prefix("_" + std::to_string(task->world_rank));
    wrapper_mpi.logs_with_prefix("_" + std::to_string(task->world_rank));

    for (Burner* burner : burners) {
        delete burner;
    }

    MPI_Finalize();

    // TODO: summarize (if -v) or output results/statistics
}

