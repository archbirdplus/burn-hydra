#pragma once

#include <mutex>
#include <flint/fmpz.h>
#include <utility>

#include "types.h"

using mutex_t = std::mutex;

typedef struct locked_fmpz {
    timed_fmpz value;
    mutex_t mutex;

    locked_fmpz();
    ~locked_fmpz();
    locked_fmpz(locked_fmpz&&) noexcept;
    locked_fmpz& operator =(locked_fmpz&&) noexcept;

    void clear();

    timed_fmpz* lock_unknown();
    void unlock();
    timed_fmpz* lock(int64_t time);
    void unlock_check(int64_t time);
} locked_fmpz;

bool locked_fmpz_synced(const locked_fmpz& lhs, const locked_fmpz& rhs);
bool locked_fmpz_check_synced(const locked_fmpz& lhs, const locked_fmpz& rhs);

#define ASSERT_SYNCED_LOCKED(lhs, rhs) { assert(locked_fmpz_check_synced((lhs), (rhs))); }

