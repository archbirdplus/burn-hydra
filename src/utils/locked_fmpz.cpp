#include "locked_fmpz.h"
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <stdexcept>

locked_fmpz::locked_fmpz() {
    value = timed_fmpz();
    value.iterations = 0;
    omp_init_lock(&mutex);
}

timed_fmpz* locked_fmpz::lock_unknown() {
    omp_set_lock(&mutex);
    return &value;
}

void locked_fmpz::unlock() {
    omp_unset_lock(&mutex);
}

timed_fmpz* locked_fmpz::lock(int64_t time) {
    omp_set_lock(&mutex);
    if (value.iterations != time) {
        std::cerr << "Locking fmpz expected time " << time << " but found " << value.iterations << std::endl;
        throw std::runtime_error("Lock on fmpz was logically unsynchronized");
    }
    return &value;
}

void locked_fmpz::unlock_check(int64_t time) {
    if (value.iterations != time) {
        std::cerr << "Unlocking fmpz expected time " << time << " but found " << value.iterations << std::endl;
        throw std::runtime_error("Unlock on fmpz was logically unsynchronized");
    }
    omp_unset_lock(&mutex);
}

void locked_fmpz::clear() {
    omp_set_lock(&mutex);
    value.clear();
    omp_unset_lock(&mutex);
}

bool locked_fmpz_synced(locked_fmpz& lhs, locked_fmpz& rhs) {
    omp_set_lock(&lhs.mutex);
    omp_set_lock(&rhs.mutex);
    bool value = timed_fmpz_synced(lhs.value, rhs.value);
    omp_unset_lock(&rhs.mutex);
    omp_unset_lock(&lhs.mutex);
    return value;
}

bool locked_fmpz_check_synced(locked_fmpz& lhs, locked_fmpz& rhs) {
    // std::cerr << "lhs " << lhs.iterations << " and rhs " << rhs.iterations << std::endl;
    omp_set_lock(&lhs.mutex);
    omp_set_lock(&rhs.mutex);
    if (timed_fmpz_synced(lhs.value, rhs.value)) {
        std::cerr << "Two locked_fmpz's are not synced: lhs at " << lhs.value.iterations << ", rhs at " << rhs.value.iterations << std::endl;
        return false;
    }
    omp_unset_lock(&rhs.mutex);
    omp_unset_lock(&lhs.mutex);
    return true;
}



