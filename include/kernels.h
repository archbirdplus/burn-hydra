#pragma once

#include <memory>

#include "state.h"

class Burner_basecase {
public:
    Context* global_ctx;
    uint32_t power;

    timed_fmpz storage;
    user_object_t user_object;

    Burner_basecase(Context*);
    ~Burner_basecase(Context*);

    void pushL(timed_fmpz* x_import);
    void step(timed_fmpz* x_export);
};

class Burner_MPI;

class Burner_singlethreaded {
public:
    Context* global_context;
    Burner_MPI* upper_context;
    uint64_t length;
    vec<uint32_t> scale_delta;
    vec<uint32_t> scale_next;
    vec<uint32_t> scale_self;
    vec<timed_fmpz> storage;
    vec<timed_fmpz> undercarry;
    vec<timed_fmpz> overcarry;

    Burner_singlethreaded(Context* global_ctx, Burner_MPI* upper_ctx, vec<uint32_t> scales, uint32_t next_scale);
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


class Burner_MPI {
public:
    Context* global_context;
    std::unique_ptr<Burner_singlethreaded> node_context;
    std::unique_ptr<Burner_basecase> basecase_context;

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

    // TODO: partial steps up to max
    // NOTE: also these steps will be different on different nodes
    uint64_t step();
};

