#include <iostream>
#include <cstdlib>

void friendly_assert(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::abort();
    }
}

void friendly_concern(bool* anyerror, bool condition, const char* message) {
    if (!condition) {
        *anyerror = true;
        std::cerr << message << std::endl;
    }
}


