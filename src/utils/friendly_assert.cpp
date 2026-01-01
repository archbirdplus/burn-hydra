#include <iostream>

void friendly_assert(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        exit(1);
    }
}

void friendly_concern(bool* anyerror, bool condition, const char* message) {
    if (!condition) {
        *anyerror = true;
        std::cerr << message << std::endl;
    }
}


