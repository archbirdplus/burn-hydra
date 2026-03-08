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
    { "s",                      required_argument,  NULL, 's' },
    { "flint-threads",          required_argument,  NULL, 'f' },
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

void parse_args(parse_results_t* parse_results, int argc, char** argv) {
    int ch;
    while((ch = getopt_long_only(argc, argv, "l:pn:i:s:f:", longopts, NULL)) != -1) {
        switch (ch) {
        case 'l':
            parse_results->layout = layout_t();
            parse_layout(&(*parse_results->layout), optarg);
            break;
        case 'p':
            parse_results->prune = true;
            break;
        case 'n':
            {
                const uint64_t iters = std::strtoull(optarg, nullptr, 10);
                parse_results->iterations = iters;
            }
            break;
        case 'i':
            {
                const uint64_t interval = std::strtoull(optarg, nullptr, 10);
                parse_results->checkpoint_interval = interval;
            }
            break;
        case 's':
            {
                const int64_t s = std::strtoll(optarg, nullptr, 10);
                parse_results->start_value = s;
            }
            break;
        case 'f':
            {
                const uint64_t threads = std::strtoull(optarg, nullptr, 10);
                parse_results->flint_threads = threads;
            }
            break;
        case 0:
            fprintf(stderr, "arg parse discovered null argument\n");
            break;
        }
    }
}

