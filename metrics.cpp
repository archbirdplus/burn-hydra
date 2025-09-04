#include <chrono>
#include <cassert>
#include <iostream>

#include "metrics.h"

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

