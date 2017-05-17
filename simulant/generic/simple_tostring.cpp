#include <string>

/*
 * GCC 4.7.3 didn't ship implementations of to_string, but did include it in some headers.
 * This file supplies some basic implementations using sstream functions
 */
#ifndef __clang__
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 7)

/* GCC 4.7 doesn't define std::to_string... for some reason, so we just hack around it here */

#include <cstdio>

namespace std {

std::string to_string(int value) {
    char buffer[64];
    ::sprintf(buffer, "%d", value);
    return std::string(buffer, buffer + 64);
}

std::string to_string(unsigned value) {
    char buffer[64];
    ::sprintf(buffer, "%u", value);
    return std::string(buffer, buffer + 64);
}

std::string to_string(long value) {
    char buffer[64];
    ::sprintf(buffer, "%ld", value);
    return std::string(buffer, buffer + 64);
}

std::string to_string(float value) {
    char buffer[64];
    ::sprintf(buffer, "%f", (double) value);
    return std::string(buffer, buffer + 64);
}

std::string to_string(double value) {
    char buffer[64];
    ::sprintf(buffer, "%f", value);
    return std::string(buffer, buffer + 64);
}


}

#endif
#endif
