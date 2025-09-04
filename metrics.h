#ifndef METRICS_H
#define METRICS_H

#include <chrono>
#include "common.h"

enum timer_class {
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
    active_time,
    _timer_classes,
};

typedef std::chrono::high_resolution_clock hydra_clock;
typedef std::chrono::time_point<hydra_clock> start_time_t;

typedef struct timers {
    std::chrono::nanoseconds total[_timer_classes];
    std::optional<start_time_t> last_start[_timer_classes];
} timers_t;

enum counter_class {
    messages_received_right,
    messages_received_right_empty,
    _counter_classes,
};

typedef struct counters {
    uint64_t total_integer_size;
    uint64_t counter[_counter_classes];
} counters_t;

typedef struct metrics {
    timers_t timers;
    counters_t counters;
} metrics_t;

metrics_t* init_metrics();

void start_timer(metrics_t*, timer_class);
void stop_timer(metrics_t*, timer_class);

void count_counter(metrics_t*, counter_class);

void dump_metrics(metrics_t*);

#endif // METRICS_H

