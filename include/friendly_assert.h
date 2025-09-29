#ifndef FRIENDLY_ASSERT_H
#define FRIENDLY_ASSERT_H

#include <iostream>

void friendly_assert(bool, const char*);

void friendly_concern(bool*, bool, const char*);

template<typename T> void friendly_concern_equal(bool* anyerror, T a, T b, const char* message) {
    if (a != b) {
        *anyerror = true;
        std::cerr << a << " != " << b << "; " << message << std::endl;
    }
}

#endif // FRIENDLY_ASSERT_H
