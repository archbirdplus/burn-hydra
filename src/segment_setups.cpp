#include <flint/flint.h>
#include <flint/fmpz.h>
#include <cstdint>
#include <iostream>
#include <cassert>
#include <vector>

#include "segment.h"
#include "common.h"
#include "metrics.h"

void friendly_assert(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        exit(1);
    }
}

void friendly_concern(bool* anyerror, bool condition, const char* message) {
    if (!condition) {
        *anyerror = true;
        std::cerr << message << std::endl;
    }
}

// Drop non-existent blocks and check for constraints.
void constrain_config(data_t* data) {
    config_t* config = data->config;
    const int world_size = data->segment->world_size;
    uint64_t* block_max = &data->config->global_block_max;
    *block_max = 0;
    std::vector<std::vector<uint64_t>> ramp = data->config->block_sizes_funnel;
    std::vector<std::vector<uint64_t>> chain = data->config->block_sizes_chain;
    int min_plat_index = ramp.size();
    int plat_len = chain.size();
    bool any_error = false;
    friendly_assert(world_size <= min_plat_index || chain.size() > 0, "Not enough config segments to assign to all processes.");
    uint64_t previous = 0;
    for (int i = 0; i < world_size; i++) {
        if (i < min_plat_index) {
            config->block_sizes_used.push_back(ramp[i]);
        } else {
            config->block_sizes_used.push_back(chain[(i-min_plat_index)%plat_len]);
        }
    }
    const auto unrolled = config->block_sizes_used;
    for (int i = 0; i < world_size; i++) {
        const size_t l = unrolled[i].size();
        for (size_t j = 0; j < l; j++) {
            uint64_t next = unrolled[i][j];
            if (next > *block_max) {
                *block_max = next;
            }
            if (j == 0 && i != 0) {
                friendly_concern(&any_error, next == previous, "Block sizes should be consistent on segment boundaries.");
            }
            friendly_concern(&any_error, previous <= next, "Decreasing block sizes are currently not supported.");
            previous = next;
        }
    }
    friendly_concern(&any_error, data->problem->iterations % ((uint64_t)1<<*block_max) == 0, "Problem iterations currently may only be multiples of the largest block size.");
    friendly_concern(&any_error, data->config->block_sizes_used.size() == static_cast<size_t>(world_size), "internal: block sizes not correctly unrolled");
    if (any_error) {
        std::cerr << "Constraints not met." << std::endl;
        exit(1);
    }
}

void init_table(vars_t* vars, uint64_t power) {
    uint64_t p3base = 1;
    for (uint64_t i = 0; i < power; i++) { p3base *= 3; }
    vars->p3base = p3base;
    vars->table_bits = power;
    basecase_table_t* table = vars->basecase_table;
    uint64_t N = (uint64_t)1<<power;
    basecase_table_t max = -1; // assume it's unsigned
    for (uint64_t i = 0; i < N; i++) {
        uint64_t n = i;
        for (uint64_t j = 0; j < power; j++) {
            n = n*3/2;
        }
        assert(n < max);
        table[i] = static_cast<basecase_table_t>(n);
    }
}

void setup_vars(data_t* data) {
    segment_t* seg = data->segment;
    int rank = seg->world_rank;
    seg->is_base_segment = rank == 0;
    seg->is_top_segment = rank == seg->world_size-1;

    const uint64_t table_bits = 17; // just trust this lol

    vars_t* vars = data->vars;
    *vars = {
        .update = 0,
        .p3 = {},
        .tmp = {},
        .stored = {},
        .basecase_table = (basecase_table_t*) malloc(((uint64_t)1<<table_bits) * sizeof(basecase_table_t)),
        .p3base = 0,
        .table_bits = 0,

        .block_size = {},
        .global_offset = {},
    };
    fmpz_init(&vars->update);
    init_table(vars, table_bits);

    std::vector<std::vector<uint64_t>> sizes = data->config->block_sizes_used;
    uint64_t offset = 0;
    for (int i = 0; i < rank; i++) {
        std::vector<uint64_t> list = sizes[i];
        for (size_t j = 0; j < list.size(); j++) {
            offset += (uint64_t)1<<list[j];
        }
    }
    const auto list = sizes[rank];
    for (size_t j = 0; j < list.size(); j++) {
        // segments keep blocks in reversed order
        // int i = list.size() - j - 1;
        data->vars->global_offset.insert(data->vars->global_offset.begin(), offset);
        data->vars->block_size.insert(data->vars->block_size.begin(), list[j]);
        offset += (uint64_t)1<<list[j];
    }

    uint64_t max_size = vars->block_size[0];
    fmpz_t r; fmpz_init_set_ui(r, 3);
    // TODO: yes, inclusive
    for (uint64_t i = 0; i <= max_size; i++) {
        // 3^(2^0) = 3^1 = 3 is the first element of p3
        fmpz_t next; fmpz_init_set(next, r);
        vars->p3.push_back(*next);
        // skip the last squaring
        if (i < max_size) {
            fmpz_mul(r, r, r);
        }
    }
    fmpz_clear(r);

    const int s = vars->block_size.size();
    for (int i = 0; i < s; i++) {
        fmpz a; fmpz_init(&a);
        vars->tmp.push_back(a);
        fmpz b; fmpz_init(&b);
        vars->stored.push_back(b);
    }

    if (seg->is_base_segment) {
        fmpz_set_ui(&vars->stored[vars->stored.size()-1], data->problem->initial);
    }
    fmpz stored = vars->stored[vars->stored.size()-1];
    flint_printf("rank %d init to %{fmpz}\n", data->segment->world_rank, &stored);
}

data_t* segment_init(problem_t* problem, config_t* config, segment_t* segment) {
    metrics_t* metrics = (metrics_t*) malloc (sizeof(metrics_t));
    data_t* data = (data_t*) malloc (sizeof(data_t));
    vars_t* vars = (vars_t*) calloc (1, sizeof(vars_t));
    init_metrics(metrics, segment->world_size < 3 ? true : segment->world_rank > 2);
    timer_start(metrics, initializing);
    *data = {
        .problem = problem,
        .config = config,
        .segment = segment,
        .vars = vars,
        .metrics = metrics,
    };
    constrain_config(data);
    setup_vars(data);
    timer_stop(metrics, initializing);
    return data;
}

