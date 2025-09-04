#include <chrono>
#include <cassert>
#include <iostream>

#include "metrics.h"

const char* timer_class_names[] = {
    "waiting to send left",
    "waiting to send left (mpi)",
    "waiting to send left (copying)",
    "waiting to recv left",
    "waiting to recv left (mpi)",
    "waiting to recv left (copying)",
    "waiting to send right",
    "waiting to send right (mpi)",
    "waiting to send right (copying)",
    "waiting to recv right",
    "waiting to recv right (mpi)",
    "waiting to recv right (copying)",
    "grinding basecase",
    "grinding chain",
    "actively",
    "uh oh",
};

const char* counter_class_names[] = {
    "messages received from the right",
    "messages received from the right, empty",
    "uh oh",
};

metrics_t* init_metrics() {
    metrics_t* metrics = (metrics_t*) malloc (sizeof(metrics_t));
    for (int i = 0; i < _timer_classes; i++) {
        metrics->timers.total[i] = std::chrono::nanoseconds::zero();
        metrics->timers.last_start[i] = std::nullopt;
    }
    // the rest are zero-initialized
    return metrics;
}

void start_timer(metrics_t* metrics, timer_class t) {
    if (auto start = metrics->timers.last_start[t]) {
        assert(false);
    } else {
        metrics->timers.last_start[t] = hydra_clock::now();
    }
}

void stop_timer(metrics_t* metrics, timer_class t) {
    if (auto start = metrics->timers.last_start[t]) {
        const auto delta = hydra_clock::now() - *start;
        if (delta < std::chrono::nanoseconds::zero()) {
            std::cerr << "Experienced time travel: " << delta.count() << " ns time elapased." << std::endl;
        }
        metrics->timers.total[t] += delta;
    } else {
        assert(false);
    }
}

void count_counter(metrics_t* metrics, counter_class t) {
    metrics->counters.counter[t] += 1;
}

void dump_metrics(metrics_t* metrics) {
    for (int i = 0; i < _timer_classes; i++) {
        const auto time = metrics->timers.total[i];
        std::chrono::duration<double> seconds = time;
        std::cout << seconds.count() << " s spent " << timer_class_names[i] << "." << std::endl;
    }
}

