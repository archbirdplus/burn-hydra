// Fluent C++ wrapper API

using vec<T> = std::vector<T>
using vecvec<T> = std::vector<std::vector<T>>
using opt<T> = std::optional<T>

typedef user_object_t uint64_t;
typedef user_object_t(void*, user_object_t, uint64_t) scan_fn_t;

typedef struct scan_config {
    scan_fn_t scan_fn;
    void* scan_context;
    uint64_t scan_block_size;
    bool scan_memoize;
} scan_config_t;

// Distinct from the definition in https://wiki.bbchallenge.org/wiki/Consistent_Collatz
// Notably closer aligned to the inductive pattern style.
// x_(n+1) = r floor(x/m) + J[x_n mod m]
typedef struct collatz_function {
    int64_t r;
    int64_t m;
    vec<int64_t> J;
} collatz_function_t;

// Config stores the information about a computation before it is executed.
class Setup {
private:
    opt<int64_t> initial;
    opt<int64_t> iterations;
    opt<int64_t> checkpoint_interval;
    opt<bool> prune;

    opt<vecvec<uint64_t>> block_sizes_ramp;
    opt<vecvec<uint64_t>> block_sizes_plat;

    opt<scan_config_t> scan_config;
public:
    Config(); // init with no defaults
    // common creatures
    static Config hydra();
    static Config bigfoot();
    Config clone() const; // init new from self

    Config from_argv();

    Config block_sizes(vecvec<uint64_t> ramp_up, vecvec<uint64_t> plat);
    Config checkpoint_interval(int64_t interval);

    // TODO: implement non-base2 collatzes
    Config consistent_collatz(int64_t r, int64_t m, vector<int64_t> J);
    Config set_initial(int64_t r);

    Config do_prune(bool prune);
    Config steps(int64_t n);

    Config scan_fn(scan_fn_t fn, uint64_t block_size, bool memoize);
    Config scan_context(void*);

    bool check() const;
    State init() const;
}

class MetaState {
public:
    int64_t initial;
    int64_t max_iterations;
    int64_t checkpoint_interval;

    vecvec<uint64_t> block_sizes;
    uint64_t block_size_max;

    scan_config_t scan_config;

    MetaState(int64_t initial, int64_t iterations, int64_t checkpoint_interval, vecvec<uint64_t> block_sizes, scan_config_t scan_config);
}

class Workspace {
public:
    vec<fmpz> pR;
    vec<fmpz> pM;
    vec<fmpz> stored;
    void* basecase_table;

    // Table memoizing g^n in terms of indices
    vec<uint64_t> scan_table;
    // user object -> table index
    std::unordered_map<user_object_t, uint64_t> scan_index_from_object;
    // table index -> user object
    vec<uint64_t> scan_object_from_index;
}

// State handles the memory needed at the time of execution.
class State {
private:
public:
    Setup setup; // TODO: this may change; remove from State?
    MetaState meta;
    Workspace work;

    State(Setup setup); // init from problem statement

    void run();
}

