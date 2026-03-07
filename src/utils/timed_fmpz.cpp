#include "types.h"
#include <cstdlib>
#include <iostream>
#include <cassert>

timed_fmpz::timed_fmpz() {
    fmpz_init_set_ui(&value, 0);
    iterations = -1;
}

void timed_fmpz::clear() {
    fmpz_clear(&value);
    iterations = -1;
}

timed_fmpz::~timed_fmpz() {
    clear();
}

timed_fmpz::timed_fmpz(timed_fmpz&& other) noexcept {
    fmpz_init_set_ui(&value, 0);
    fmpz_swap(&value, &other.value);
    iterations = other.iterations;
    other.iterations = -1;
}

timed_fmpz& timed_fmpz::operator =(timed_fmpz&& other) noexcept {
    if (this != &other) {
        this->clear();
        fmpz_init(&this->value);
        fmpz_swap(&this->value, &other.value);
        this->iterations = other.iterations;
        other.iterations = -1;
        other.clear();
    }
    return *this;
}

void timed_fmpz_swap(timed_fmpz& lhs, timed_fmpz& rhs) {
    fmpz_swap(&lhs.value, &rhs.value);
    std::swap(lhs.iterations, rhs.iterations);
}

bool timed_fmpz_synced(const timed_fmpz& lhs, const timed_fmpz& rhs) {
    return lhs.iterations == rhs.iterations && lhs.iterations >= 0 && rhs.iterations >= 0;
}

bool timed_fmpz_check_synced(const timed_fmpz& lhs, const timed_fmpz& rhs) {
    // std::cerr << "lhs " << lhs.iterations << " and rhs " << rhs.iterations << std::endl;
    if (!timed_fmpz_synced(lhs, rhs)) {
        std::cerr << "Two timed_fmpz's are not synced: lhs at " << lhs.iterations << ", rhs at " << rhs.iterations << std::endl;
        return false;
    }
    return true;
}


