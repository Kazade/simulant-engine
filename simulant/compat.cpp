
#include <cstdio>
#include <cstdlib>

#include "compat.h"

namespace std {

#if defined(_arch_dreamcast) || defined(__PSP__)

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

std::string to_string(unsigned long value) {
    char buffer[64];
    auto c = ::sprintf(buffer, "%luld", value);
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

#endif

#if defined(__PSP__)

int stoi(const std::string& str) {
    return atoi(str.c_str());
}

float stof(const std::string& str) {
    return atof(str.c_str());
}

#endif
}



