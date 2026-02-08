#include <chrono>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <fstream>
#include <stdexcept>

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
    "gather communication",
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

Metrics::Metrics(bool full_logs) {
    for (int i = 0; i < _timer_classes; i++) {
        this->timers.total[i] = std::chrono::nanoseconds::zero();
        this->timers.last_start[i] = std::nullopt;
    }
    timers_t* timers = &this->timers;
    for (int i = 0; i < _timer_classes; i++) {
        timers->intervals[i] = std::nullopt;
    }
    #ifndef NO_PLOT_LOGS
    timers->intervals[active_time] = std::vector<start_stop_t>();
    timers->intervals[initializing] = std::vector<start_stop_t>();
    timers->intervals[waiting_send_left] = std::vector<start_stop_t>();
    timers->intervals[waiting_recv_left] = std::vector<start_stop_t>();
    timers->intervals[gather_communication] = std::vector<start_stop_t>();
    if (full_logs) {
        timers->intervals[waiting_send_right] = std::vector<start_stop_t>();
        timers->intervals[waiting_recv_right] = std::vector<start_stop_t>();
        timers->intervals[grinding_chain] = std::vector<start_stop_t>();
    }
    #else
    (void)full_logs;
    #endif
    // the rest should be zero-initialized
}

void Metrics::start_timer(timer_class t) {
    if (auto start = timers.last_start[t]) {
        std::cout << "ouch: Timer was started twice." << std::endl;
        assert(false);
    } else {
        timers.last_start[t] = hydra_clock::now();
    }
}

void Metrics::stop_timer(timer_class t) {
    if (auto start = timers.last_start[t]) {
        const auto stop = hydra_clock::now();
        const auto delta = stop - *start;
        if (delta < std::chrono::nanoseconds::zero()) {
            std::cout << "ouch: Experienced time travel: " << delta.count() << " ns time elapased." << std::endl;
        }
        timers.total[t] += delta;
        timers.last_start[t] = std::nullopt;
        #ifndef NO_PLOT_LOGS
        if (timers.intervals[t] != std::nullopt) {
            timers.intervals[t].value().push_back({*start, stop});
        }
        #endif
    } else {
        std::cout << "ouch: Timer was stopped twice." << std::endl;
        assert(false);
    }
}

void Metrics::count(counter_class t) {
    counters.counter[t] += 1;
}

void Metrics::dump_as_rank(int rank) {
    std::string filename {"rank"};
    filename.append(std::to_string(rank));
    filename.append(".json");
    std::fstream f {filename, std::ios::out};
    std::cout << "Some metrics were tracked:" << std::endl;
    for (int t = 0; t < _timer_classes; t++) {
        const auto time = timers.total[t];
        std::chrono::duration<double> seconds = time;
        std::cout << "\t" << seconds.count() << " s spent " << timer_class_names[t] << "." << std::endl;
    }
    for (int i = 0; i < _counter_classes; i++) {
        const auto counts = counters.counter[i];
        std::cout << "\t" << counts << " " << counter_class_names[i] << "." << std::endl;
    }
    #ifndef NO_PLOT_LOGS
    // do a bit of json
    // { "timer_class_a": [[start, stop], [start, stop]...], ... }
    if (timers.intervals[initializing] == std::nullopt) {
        std::cout << "init timer is nullopt, skipping file write" << std::endl;
        return;
    }
    std::cout << "Dumping json timer intervals." << std::endl;
    if (timers.intervals[active_time]->size() == 0) {
        throw std::runtime_error("Internal error: did not begin active timer");
    }
    const start_time_t first_start = (*timers.intervals[active_time])[0].start;
    if (rank > 0) {
        f << ",";
    }
    f << "\"rank " << rank << "\": {";
    for (int t = 0; t < _timer_classes; t++) {
        if (std::optional<std::vector<start_stop_t>> intervals = timers.intervals[t]) {
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

