#include <chrono>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <fstream>

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

void init_metrics(metrics_t* metrics, bool full_logs) {
    // for some reason malloc here is _really_ bad, and puts
    // a brk instead of this whole function specifically in -O2
    for (int i = 0; i < _timer_classes; i++) {
        metrics->timers.total[i] = std::chrono::nanoseconds::zero();
        metrics->timers.last_start[i] = std::nullopt;
    }
    timers_t* timers = &metrics->timers;
    for (int i = 0; i < _timer_classes; i++) {
        timers->intervals[i] = std::nullopt;
    }
    #ifndef NO_PLOT_LOGS
    timers->intervals[initializing] = std::vector<start_stop_t>();
    timers->intervals[waiting_send_left] = std::vector<start_stop_t>();
    timers->intervals[waiting_recv_left] = std::vector<start_stop_t>();
    if (full_logs) {
        timers->intervals[waiting_send_right] = std::vector<start_stop_t>();
        timers->intervals[waiting_recv_right] = std::vector<start_stop_t>();
        timers->intervals[grinding_chain] = std::vector<start_stop_t>();
    }
    #endif
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
        const auto stop = hydra_clock::now();
        const auto delta = stop - *start;
        if (delta < std::chrono::nanoseconds::zero()) {
            std::cout << "ouch: Experienced time travel: " << delta.count() << " ns time elapased." << std::endl;
        }
        metrics->timers.total[t] += delta;
        metrics->timers.last_start[t] = std::nullopt;
        #ifndef NO_PLOT_LOGS
        if (metrics->timers.intervals[t] != std::nullopt) {
            metrics->timers.intervals[t].value().push_back({*start, stop});
        }
        #endif
    } else {
        std::cout << "ouch: Timer was stopped twice." << std::endl;
        assert(false);
    }
}

void counter_count(metrics_t* metrics, counter_class t) {
    metrics->counters.counter[t] += 1;
}

void dump_metrics(metrics_t* metrics, int rank) {
    std::string filename {"rank"};
    filename.append(std::to_string(rank));
    filename.append(".json");
    std::fstream f {filename, std::ios::out};
    std::cout << "Some metrics were tracked:" << std::endl;
    for (int t = 0; t < _timer_classes; t++) {
        const auto time = metrics->timers.total[t];
        std::chrono::duration<double> seconds = time;
        std::cout << "\t" << seconds.count() << " s spent " << timer_class_names[t] << "." << std::endl;
    }
    for (int i = 0; i < _counter_classes; i++) {
        const auto counts = metrics->counters.counter[i];
        std::cout << "\t" << counts << " " << counter_class_names[i] << "." << std::endl;
    }
    #ifndef NO_PLOT_LOGS
    // do a bit of json
    // { "timer_class_a": [[start, stop], [start, stop]...], ... }
    if (metrics->timers.intervals[initializing] == std::nullopt) {
        std::cout << "init timer is nullopt, skipping file write" << std::endl;
        return;
    }
    std::cout << "Dumping json timer intervals." << std::endl;
    const start_time_t first_start = (*metrics->timers.intervals[initializing])[0].start;
    if (rank > 0) {
        f << ",";
    }
    f << "\"rank " << rank << "\": {";
    for (int t = 0; t < _timer_classes; t++) {
        if (std::optional<std::vector<start_stop_t>> intervals = metrics->timers.intervals[t]) {
            if (t > 0) {
                f << ",";
            }
            f << "\"" << timer_class_names[t] << "\": [";
            const size_t count = intervals->size();
            for (size_t i = 0; i < count; i++) {
                if (i > 0) {
                    f << ",";
                }
                const double start = seconds((*intervals)[i].start - first_start);
                const double stop = seconds((*intervals)[i].stop - first_start);
                f << "[" << start << "," << stop << "]";
            }
            f << "]";
        }
    }
    f << "}";
    f.flush();
    #endif
}

