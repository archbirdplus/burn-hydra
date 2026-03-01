#include "locked_fmpz.h"
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <stdexcept>

locked_fmpz::locked_fmpz() {
    value = timed_fmpz();
    value.iterations = 0;
}

locked_fmpz::~locked_fmpz() {
    value.clear();
}

locked_fmpz::locked_fmpz(locked_fmpz&& rhs) noexcept {
    mutex.lock();
    rhs.mutex.lock();
    std::swap(value, rhs.value);
    rhs.mutex.unlock();
}

locked_fmpz& locked_fmpz::operator =(locked_fmpz&& other) noexcept {
    if (this != &other) {
        this->mutex.lock();
        other.mutex.lock();
        std::swap(value, other.value);
        other.mutex.unlock();
        this->mutex.unlock();
    }
    return *this;
}

timed_fmpz* locked_fmpz::lock_unknown() {
    mutex.lock();
    return &value;
}

void locked_fmpz::unlock() {
    mutex.unlock();
}

timed_fmpz* locked_fmpz::lock(int64_t time) {
    mutex.lock();
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
    mutex.unlock();
}

void locked_fmpz::clear() {
    mutex.lock();
    value = timed_fmpz();
    mutex.unlock();
}

bool locked_fmpz_synced(locked_fmpz& lhs, locked_fmpz& rhs) {
    lhs.mutex.lock();
    rhs.mutex.lock();
    bool value = timed_fmpz_synced(lhs.value, rhs.value);
    rhs.mutex.lock();
    lhs.mutex.lock();
    return value;
}

bool locked_fmpz_check_synced(locked_fmpz& lhs, locked_fmpz& rhs) {
    // std::cerr << "lhs " << lhs.iterations << " and rhs " << rhs.iterations << std::endl;
    lhs.mutex.lock();
    rhs.mutex.lock();
    if (timed_fmpz_synced(lhs.value, rhs.value)) {
        std::cerr << "Two locked_fmpz's are not synced: lhs at " << lhs.value.iterations << ", rhs at " << rhs.value.iterations << std::endl;
        return false;
    }
    rhs.mutex.unlock();
    lhs.mutex.unlock();
    return true;
}



