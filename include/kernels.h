#pragma once

#include <memory>

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

    virtual void pushR(timed_fmpz* x_export) = 0;
    virtual void pullR(timed_fmpz* x_export) = 0;
    virtual void pushL(timed_fmpz* x_export) = 0;
    virtual void pullL(timed_fmpz* x_export) = 0;
};

class Burner_simple: public Burner, public Runnable {
public:
    Context* global_context;
    Wrapper* upper_context;
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

class Wrapper_MPI: public Wrapper {
public:
    Context* global_context;
    Runnable* local_burner;

    vec<uint32_t> local_scales;
    uint32_t local_next_scale;

    int world_size;
    int world_rank;

    bool can_push_left;
    bool can_push_right;

    Wrapper_MPI(Context* global_ctx);
    ~Wrapper_MPI();

    subscription_t add_subscriber(Runnable* local);

    void pushR(timed_fmpz* x_export);
    void pullR(timed_fmpz* x_import);

    void pushL(timed_fmpz* x_export);
    void pullL(timed_fmpz* x_import);

    uint64_t step();
    void run_until(uint64_t steps);
};

