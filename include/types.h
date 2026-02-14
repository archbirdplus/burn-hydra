#pragma once

#include <vector>
#include <optional>
#include <flint/fmpz.h>
#include <cassert>
#include <cstdint>

template <class T>
using vec = std::vector<T>;
template <class T>
using vecvec = std::vector<std::vector<T>>;
template <class T>
using opt = std::optional<T>;

using user_object_t = uint64_t;
using scan_fn_t = user_object_t(*)(void*, user_object_t, uint64_t);

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

typedef struct timed_fmpz {
    fmpz value;
    int64_t iterations;

    timed_fmpz();
    ~timed_fmpz();
    timed_fmpz(timed_fmpz&&) noexcept;
    timed_fmpz& operator =(timed_fmpz&&) noexcept;

    void clear();
} timed_fmpz;

bool timed_fmpz_synced(const timed_fmpz& lhs, const timed_fmpz& rhs);
bool timed_fmpz_check_synced(const timed_fmpz& lhs, const timed_fmpz& rhs);

#define ASSERT_SYNCED(lhs, rhs) { assert(timed_fmpz_check_synced((lhs), (rhs))); }

