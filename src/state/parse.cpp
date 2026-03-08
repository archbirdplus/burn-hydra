#include <cassert>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <getopt.h>

#include <stdexcept>
#include <iostream>

#include "types.h"
#include "fluent.h"
#include "parse.h"

// --config '8-18:20-22/22-24//24:24:24'
// --prune
// --iterations 1234567
// --checkpoint-interval 65536
// -s 3

static struct option longopts[] = {
    { "layout",                 required_argument,  NULL, 'l' },
    { "prune",                  no_argument,        NULL, 'p' },
    { "iterations",             required_argument,  NULL, 'n' },
    { "checkpoint-interval",    required_argument,  NULL, 'i' },
    { "x",                      required_argument,  NULL, 'x' },
    { NULL,                     0,                  NULL,  0  },
};

void parse_layout(layout_t* layout, const char* const arg) {
    vecvec<uint64_t>* thread_breaks = &layout->thread_breaks;
    vecvec<uint64_t>* block_sizes_ramp = &layout->block_sizes_ramp;
    vecvec<uint64_t>* block_sizes_plat = &layout->block_sizes_plat;

    bool parsing_ramp = true;
    bool parsing_interval = false;
    bool segment_empty = false;
    vec<uint64_t> segment_sizes = {};
    vec<uint64_t> segment_breaks = {};
    char* ptr = (char*) arg;
    char ch;
    bool done = false;
    while (!done) {
        ch = *ptr;
        switch (ch) {
        case ':':
            segment_breaks.push_back(segment_sizes.size()-1);
            break;
        case ',':
            break;
        case '-':
            parsing_interval = true;
            break;
        case '/':
        case 0:
            if (parsing_interval) {
                throw std::runtime_error("Cannot parse open interval with no previous size.");
            }
            if (segment_empty) {
                parsing_ramp = false;
            } else {
                auto sizes = parsing_ramp ? block_sizes_ramp : block_sizes_plat;
                sizes->push_back(segment_sizes);
                thread_breaks->push_back(segment_breaks);
            }
            segment_empty = true;
            segment_sizes = {};
            segment_breaks = {};
            if (ch == 0) done = true;
            break;
        default:
            segment_empty = false;
            uint64_t size = std::strtoull(ptr, &ptr, 10);
            if (parsing_interval) {
                uint64_t prev;
                if (segment_sizes.size() > 0) {
                    prev = segment_sizes[segment_sizes.size()-1];
                } else if (parsing_ramp && block_sizes_ramp->size() > 0) {
                    vec<uint64_t> prev_seg = (*block_sizes_ramp)[block_sizes_ramp->size()-1];
                    prev = prev_seg[prev_seg.size()-1];
                } else {
                    throw std::runtime_error("Cannot parse open interval with no previous size.");
                }
                std::cout << "prev is " << prev << " and size is " << size << std::endl;
                if (prev < size) {
                    for (uint64_t s = prev+1; s <= size; s++) {
                        segment_sizes.push_back(s);
                    }
                } else if (prev > size) {
                    for (uint64_t s = prev-1; s >= size; s--) {
                        segment_sizes.push_back(s);
                    }
                } else {
                    segment_sizes.push_back(size);
                }
                parsing_interval = false;
            } else {
                segment_sizes.push_back(size);
            }
            ptr--;
            break;
        }
        ptr++;
    }
}

/*
void parse_args(problem_t* problem, config_t* config, int argc, char** argv) {
    bool x_set = false;
    bool config_set = false;
    bool iterations_set = false;
    bool checkpoint_set = false;
    int ch;
    while((ch = getopt_long_only(argc, argv, "c:pn:i:x:", longopts, NULL)) != -1) {
        switch (ch) {
        case 'c':
            parse_config(config, optarg);
            config_set = true;
            break;
        case 'p':
            config->prune_bits = true;
            break;
        case 'n':
            {
                const int64_t iters = std::strtoll(optarg, nullptr, 10);
                problem->iterations = iters;
            }
            iterations_set = true;
            break;
        case 'i':
            {
                const int64_t interval = std::strtoll(optarg, nullptr, 10);
                config->checkpoint_interval = interval;
            }
            checkpoint_set = true;
            break;
        case 'x':
            {
                const uint64_t x = std::strtoull(optarg, nullptr, 10);
                problem->initial = x;
            }
            x_set = true;
            break;
        case 0:
            fprintf(stderr, "arg parse discovered null argument\n");
            break;
        }
    }
    if (!x_set) {
        problem->initial = 3;
    }
    if (!config_set) {
        fprintf(stderr, "burn_hydra requires a configuration string.\n");
    }
    if (!iterations_set) {
        fprintf(stderr, "Number of iterations was not specified.\n");
    }
    if (!checkpoint_set) {
        config->checkpoint_interval = 0;
    }
}
*/

/*
void test_parse_args() {
    problem_t problem;
    config_t config = {
        .block_sizes_funnel = {},
        .block_sizes_chain = {},
        .block_sizes_used = {},
        .global_block_max = 0,
        .prune_bits = 0,
        .checkpoint_interval = 0,
    };
    // These (char*) casts are doing a lot of heavy lifting. Hopefully
    // getopt doesn't modify them.
    std::vector<char*> vec = { NULL, (char*)"--config=9-27,3-4/5-6", (char*)"--prune", (char*)"--iterations", (char*)"420", (char*)"--checkpoint-interval", (char*)"39", (char*)"--x", (char*)"5" };
    char** argv = &vec[0];
    parse_args(&problem, &config, 9, argv);
    assert(problem.initial == 5);
    assert(problem.iterations == 420);

    assert(std::vector<std::vector<uint64_t>>({{9, 27}, {3, 4}}) == config.block_sizes_funnel);
    assert(std::vector<std::vector<uint64_t>>({{5, 6}}) == config.block_sizes_chain);
    assert(config.global_block_max == 27);

    assert(config.prune_bits == true);
    assert(config.checkpoint_interval == 39);

    config = {
        .block_sizes_funnel = {},
        .block_sizes_chain = {},
        .block_sizes_used = {},
        .global_block_max = 0,
        .prune_bits = 0,
        .checkpoint_interval = 0,
    };
    // apparently -c= does not work, but abbreviations in general do
    vec = { NULL, (char*)"-c", (char*)"9-27,3-4/5-6", (char*)"-p", (char*)"-n", (char*)"420", (char*)"-i", (char*)"39", (char*)"-x", (char*)"5" };
    argv = &vec[0];
    optind = 0; // getopt is so jank
    #ifdef optreset
    optreset = 1;
    #endif // optreset
    // skip over the -x
    parse_args(&problem, &config, 8, argv);
    assert(problem.initial == 3);
    assert(problem.iterations == 420);

    assert(std::vector<std::vector<uint64_t>>({{9, 27}, {3, 4}}) == config.block_sizes_funnel);
    assert(std::vector<std::vector<uint64_t>>({{5, 6}}) == config.block_sizes_chain);
    assert(config.global_block_max == 27);

    assert(config.prune_bits == true);
    assert(config.checkpoint_interval == 39);
}
*/

