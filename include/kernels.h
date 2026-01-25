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

    void step(timed_fmpz* x_export, timed_fmpz* x_import);
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

    void syncR(uint64_t n);

    void syncL(uint64_t n);

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

    void syncR(timed_fmpz* x_export, timed_fmpz* x_import);

    void syncL(timed_fmpz* x_export, timed_fmpz* x_import);

    // TODO: partial steps up to max
    // NOTE: also these steps will be different on different nodes
    uint64_t step();
};

