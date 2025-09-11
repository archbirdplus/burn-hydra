#include <chrono>
#include <cassert>
#include <iostream>

#include "metrics.h"

const char* timer_class_names[] = {
    "initializing variables",
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
    "messages received from the right, nonempty",
    "uh oh",
};

start_time_t nanos() {
    return hydra_clock::now();
}

double seconds(std::chrono::nanoseconds time) {
    std::chrono::duration<double> seconds = time;
    return seconds.count();
}

void init_metrics(metrics_t* metrics) {
    // for some reason malloc here is _really_ bad, and puts
    // a brk instead of this whole function specifically in -O2
    for (int i = 0; i < _timer_classes; i++) {
        metrics->timers.total[i] = std::chrono::nanoseconds::zero();
        metrics->timers.last_start[i] = std::nullopt;
    }
    // the rest are zero-initialized
}

void timer_start(metrics_t* metrics, timer_class t) {
    if (auto start = metrics->timers.last_start[t]) {
        std::cout << "ouch: Timer was started twice." << std::endl;
        assert(false);
    } else {
        metrics->timers.last_start[t] = hydra_clock::now();
    }
}

void timer_stop(metrics_t* metrics, timer_class t) {
    if (auto start = metrics->timers.last_start[t]) {
        const auto delta = hydra_clock::now() - *start;
        if (delta < std::chrono::nanoseconds::zero()) {
            std::cout << "ouch: Experienced time travel: " << delta.count() << " ns time elapased." << std::endl;
        }
        metrics->timers.total[t] += delta;
        metrics->timers.last_start[t] = std::nullopt;
    } else {
        std::cout << "ouch: Timer was stopped twice." << std::endl;
        assert(false);
    }
}

void counter_count(metrics_t* metrics, counter_class t) {
    metrics->counters.counter[t] += 1;
}

void dump_metrics(metrics_t* metrics) {
    std::cout << "Some metrics were tracked:" << std::endl;
    for (int i = 0; i < _timer_classes; i++) {
        const auto time = metrics->timers.total[i];
        std::chrono::duration<double> seconds = time;
        std::cout << "\t" << seconds.count() << " s spent " << timer_class_names[i] << "." << std::endl;
    }
    for (int i = 0; i < _counter_classes; i++) {
        const auto counts = metrics->counters.counter[i];
        std::cout << "\t" << counts << " " << counter_class_names[i] << "." << std::endl;
    }
}

