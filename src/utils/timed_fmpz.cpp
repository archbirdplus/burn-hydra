#include "types.h"
#include <cstdlib>
#include <iostream>
#include <cassert>

timed_fmpz::timed_fmpz() {
    fmpz_init_set_ui(&fmpz, 0);
    iterations = -1;
}

timed_fmpz::~timed_fmpz() {
    std::cout << "timed_fmpz clearing" << std::endl;
    fmpz_clear(&fmpz);
}

bool timed_fmpz_synced(const timed_fmpz& lhs, const timed_fmpz& rhs) {
    return lhs.iterations == rhs.iterations && lhs.iterations >= 0 && rhs.iterations >= 0;
}

bool timed_fmpz_check_synced(const timed_fmpz& lhs, const timed_fmpz& rhs) {
    if (!timed_fmpz_synced(lhs, rhs)) {
        std::cerr << "Two timed_fmpz's are not synced: lhs at " << lhs.iterations << ", rhs at " << rhs.iterations << std::endl;
        return false;
    }
    return true;
}


