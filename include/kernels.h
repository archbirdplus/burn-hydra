#pragma once

#include <memory>

#include "state.h"

class Basecase_simple {
public:
    Context* global_ctx;
    uint32_t power;

    timed_fmpz storage;
    user_object_t user_object;

    Basecase_simple(Context*);
    virtual ~Basecase_simple();

    virtual void pushL(timed_fmpz* x_import);
    virtual void step(timed_fmpz* x_export);
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

    void step(timed_fmpz* x_export);
};

class Burner_MPI;

class Burner_singlethreaded {
public:
    Context* global_context;
    std::unique_ptr<Burner_MPI> upper_context;
    std::unique_ptr<Basecase_simple> basecase_context;
    uint64_t length;
    vec<uint32_t> scale_delta;
    vec<uint32_t> scale_next;
    vec<uint32_t> scale_self;
    vec<timed_fmpz> storage;
    vec<timed_fmpz> undercarry;
    vec<timed_fmpz> overcarry;

    Burner_singlethreaded(Context* global_ctx, std::unique_ptr<Burner_MPI> upper_ctx, std::unique_ptr<Basecase_simple> basecase_ctx);
    ~Burner_singlethreaded();

    void tick(uint64_t n);

    void exchange(uint64_t n);

    void pushR(uint64_t n);
    void pullR(uint64_t n);

    void pushL(uint64_t n);
    void pullL(uint64_t n);

    void recurse(int64_t n);

    // TODO: partial steps up to max
    uint64_t step();
};


//pushR actually depends on basecasetype
class Burner_MPI {
public:
    Context* global_context;

    vec<uint32_t> local_scales;
    uint32_t local_next_scale;

    int world_size;
    int world_rank;

    bool can_push_left;
    bool can_push_right;

    Burner_MPI(Context* global_ctx);
    ~Burner_MPI();

    void pushR(timed_fmpz* x_export);
    void pullR(timed_fmpz* x_import);

    void pushL(timed_fmpz* x_export);
    void pullL(timed_fmpz* x_import);

};

