#pragma once

#include <chrono>
#include <optional>
#include <vector>

enum timer_class {
    initializing,
    waiting_send_left,
    waiting_send_left_mpi,
    waiting_send_left_copy,
    waiting_recv_left,
    waiting_recv_left_mpi,
    waiting_recv_left_copy,
    waiting_send_right,
    waiting_send_right_mpi,
    waiting_send_right_copy,
    waiting_recv_right,
    waiting_recv_right_mpi,
    waiting_recv_right_copy,
    grinding_basecase,
    grinding_chain,
    gather_communication,
    active_time,
    _timer_classes,
};

typedef std::chrono::high_resolution_clock hydra_clock;
typedef std::chrono::time_point<hydra_clock> start_time_t;

typedef struct start_stop_pair {
    start_time_t start;
    start_time_t stop;
} start_stop_t;

typedef struct timers {
    std::chrono::nanoseconds total[_timer_classes];
    std::optional<start_time_t> last_start[_timer_classes];
    std::optional<std::vector<start_stop_t>> intervals[_timer_classes];
} timers_t;

enum counter_class {
    messages_received_right,
    messages_received_right_nonempty,
    _counter_classes,
};

typedef struct counters {
    uint64_t total_integer_size;
    uint64_t counter[_counter_classes];
} counters_t;

start_time_t nanos();
double seconds(std::chrono::nanoseconds);

class Metrics {
public:
    timers_t timers;
    counters_t counters;

    Metrics(bool full_logs);

    void start_timer(timer_class);
    void stop_timer(timer_class);

    void count(counter_class);

    void dump_as_rank(int rank);
};

