
#include <cstdio>
#include "compat.h"


#ifndef __clang__
// FIXME: these version checks may not be correct, I'm not sure
// which version improved the define checking
#if (__GNUC__ == 4 || __GNUC__ == 5 || __GNUC__ == 6 || __GNUC__ == 7)
#ifndef _GLIBCXX_USE_C99_STDIO

namespace std {

std::string to_string(int value) {
    char buffer[64];
    auto c = ::sprintf(buffer, "%d", value);
    return std::string(buffer, buffer + c);
}

std::string to_string(unsigned value) {
    char buffer[64];
    auto c = ::sprintf(buffer, "%u", value);
    return std::string(buffer, buffer + c);
}

std::string to_string(long value) {
    char buffer[64];
    auto c = ::sprintf(buffer, "%ld", value);
    return std::string(buffer, buffer + c);
}

std::string to_string(float value) {
    char buffer[64];
    auto c = ::sprintf(buffer, "%f", (double) value);
    return std::string(buffer, buffer + c);
}

std::string to_string(double value) {
    char buffer[64];
    auto c = ::sprintf(buffer, "%f", value);
    return std::string(buffer, buffer + c);
}


}


#endif
#endif
#endif
