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

class Burner_MPI;

class Burner {
public:
    virtual ~Burner() {}
    virtual uint64_t step() { return 0; }
};

class Burner_singlethreaded: public Burner {
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
    virtual ~Burner_singlethreaded();

    virtual void tick(int64_t n);

    virtual void exchange(int64_t n);

    virtual void pushR(int64_t n);
    virtual void pullR(int64_t n);

    virtual void pushL(int64_t n);
    virtual void pullL(int64_t n);

    virtual void recurse(int64_t n);

    // TODO: partial steps up to max
    virtual uint64_t step();
};

class Burner_m2exp: public Burner_singlethreaded {
public:
    uint64_t mlog;
    Burner_m2exp(Context* global_ctx, std::unique_ptr<Burner_MPI> upper_ctx, std::unique_ptr<Basecase_simple> basecase_ctx);
    virtual ~Burner_m2exp();

    virtual void tick(int64_t n) override;

    virtual void pushL(int64_t n) override;

    // virtual void recurse(int64_t n) override;

    uint64_t step() override;
};

class Burner_openmp: public Burner {
public:
    Context* global_context;
    std::unique_ptr<Burner_MPI> upper_context;
    std::unique_ptr<Basecase_simple> basecase_context;
    uint64_t length;
    vec<uint32_t> scale_delta;
    vec<uint32_t> scale_next;
    vec<uint32_t> scale_self;
    vec<locked_fmpz> storage;
    vec<locked_fmpz> undercarry;
    vec<locked_fmpz> overcarry;

    Burner_openmp(Context* global_ctx, std::unique_ptr<Burner_MPI> upper_ctx, std::unique_ptr<Basecase_simple> basecase_ctx);
    ~Burner_openmp();
    void tick(int64_t n);

    void exchange(int64_t n);

    void pushR(int64_t n);
    void pullR(int64_t n);

    void pushL(int64_t n);
    void pullL(int64_t n);

    void recurse(int64_t n);

    // TODO: partial steps up to max
    uint64_t step() override;
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

