#pragma once

#include <memory>
#include <thread>

#include "metrics.h"
#include "state.h"
#include "locked_fmpz.h"

class Basecase_simple {
public:
    Context* global_ctx;
    uint32_t power;

    timed_fmpz storage;
    user_object_t user_object;

    Basecase_simple(Context*);
    virtual ~Basecase_simple();

    virtual void pushL(timed_fmpz* x_import);
    virtual void pullL(timed_fmpz* x_export);
    virtual void tick();
};

class Basecase_m2exp: public Basecase_simple {
public:
    uint32_t mlog;

    Basecase_m2exp(Context*);

    void pushL(timed_fmpz* x_import);
    void step(timed_fmpz* x_export);
};

class Basecase_table: public Basecase_simple {
public:
    Basecase_table(Context*);

    virtual void tick() override;
};

typedef struct subscription {
    vec<uint32_t> scales;
    uint32_t next_scale;
    bool can_push_right;
    bool can_push_left;
    uint64_t id;
} subscription_t;

class Runnable {
public:
    virtual ~Runnable() {};
    virtual void run_until(uint64_t end) = 0;
    virtual void logs_with_prefix(std::string prefix) = 0;
};

class Burner {
public:
    virtual ~Burner() {};
    virtual uint64_t step() = 0;
};

class Wrapper {
public:
    virtual ~Wrapper() {};
    virtual subscription_t add_subscriber(Runnable* burner) = 0;

    virtual void pushR(uint64_t id, timed_fmpz* x_export) = 0;
    virtual void pullR(uint64_t id, timed_fmpz* x_export) = 0;
    virtual void pushL(uint64_t id, timed_fmpz* x_export) = 0;
    virtual void pullL(uint64_t id, timed_fmpz* x_export) = 0;
};

class Burner_simple: public Burner, public Runnable {
public:
    Context* global_context;
    Wrapper* upper_context;
    std::unique_ptr<Metrics> metrics;
    subscription_t subscription;
    std::unique_ptr<Basecase_simple> basecase_context;
    uint64_t length;
    vec<uint32_t> scale_delta;
    vec<uint32_t> scale_next;
    vec<uint32_t> scale_self;
    vec<timed_fmpz> storage;
    vec<timed_fmpz> undercarry;
    vec<timed_fmpz> overcarry;

    Burner_simple(Context* global_ctx, Wrapper* upper_ctx, std::unique_ptr<Basecase_simple> basecase_ctx);
    virtual ~Burner_simple();

    virtual void tick(int64_t n);

    virtual void exchange(int64_t n);

    virtual void pushR(int64_t n);
    virtual void pullR(int64_t n);

    virtual void pushL(int64_t n);
    virtual void pullL(int64_t n);

    virtual void recurse(int64_t n);

    virtual uint64_t step();
    virtual void run_until(uint64_t end);

    virtual void logs_with_prefix(std::string prefix);
};

class Burner_m2exp: public Burner_simple {
public:
    uint64_t mlog;
    Burner_m2exp(Context* global_ctx, Wrapper* upper_ctx, std::unique_ptr<Basecase_simple> basecase_ctx);
    virtual ~Burner_m2exp();

    virtual void tick(int64_t n) override;

    virtual void pushL(int64_t n) override;

    uint64_t step() override;
};

enum carry_state { taken, putten };

typedef struct thread_break {
    uint64_t right_high;
    locked_fmpz undercarry;
    locked_fmpz overcarry;
    carry_state undercarry_state;
    carry_state overcarry_state;
    std::condition_variable hold_under;
    std::condition_variable hold_over;

    thread_break(uint64_t index);
    thread_break(thread_break&& other) noexcept;
    thread_break& operator =(thread_break&& other) noexcept;
} thread_break_t;

typedef struct thread_section {
    uint64_t high;
    uint64_t low;
    uint64_t id;
} thread_section_t;

class Wrapper_threads: public Wrapper, public Runnable {
public:
    Context* global_context;
    Wrapper* upper_context;
    std::unique_ptr<Metrics> metrics;
    subscription_t subscription;

    uint64_t thread_count;
    vec<Runnable*> thread_burners;
    vec<thread_section_t> thread_sections;
    vec<thread_break_t> thread_breaks;

    vec<uint32_t> thread_scales;
    uint32_t next_scale;

    Wrapper_threads(Context* global_ctx, Wrapper* wrapper);
    ~Wrapper_threads();

    subscription_t add_subscriber(Runnable* thread);

    void pushR(uint64_t id, timed_fmpz* x_export);
    void pullR(uint64_t id, timed_fmpz* x_import);

    void pushL(uint64_t id, timed_fmpz* x_export);
    void pullL(uint64_t id, timed_fmpz* x_import);

    void run_thread(thread_section_t section, uint64_t end);
    void run_until(uint64_t steps);

    void logs_with_prefix(std::string prefix);
};


class Wrapper_MPI: public Wrapper {
public:
    Context* global_context;
    Runnable* local_burner;
    std::unique_ptr<Metrics> metrics;

    vec<uint32_t> local_scales;
    uint32_t local_next_scale;

    int world_size;
    int world_rank;

    bool can_push_left;
    bool can_push_right;

    Wrapper_MPI(Context* global_ctx);
    ~Wrapper_MPI();

    subscription_t add_subscriber(Runnable* local);

    void pushR(uint64_t id, timed_fmpz* x_export);
    void pullR(uint64_t id, timed_fmpz* x_import);

    void pushL(uint64_t id, timed_fmpz* x_export);
    void pullL(uint64_t id, timed_fmpz* x_import);

    void run_until(uint64_t steps);

    void logs_with_prefix(std::string prefix);
};

