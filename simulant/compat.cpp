
#include <cstdio>
#include <cstdlib>

#include "compat.h"
#include "types.h"

namespace smlt {

const static int32_t _BUFFER_SIZE = 64;
static char buffer[_BUFFER_SIZE] = {0};

std::string to_string(int value) {
    auto c = ::sprintf(buffer, "%d", value);
    return std::string(buffer, buffer + c);
}

std::string to_string(unsigned value) {
    auto c = ::sprintf(buffer, "%u", value);
    return std::string(buffer, buffer + c);
}

std::string to_string(unsigned long value) {
    auto c = ::sprintf(buffer, "%luld", value);
    return std::string(buffer, buffer + c);
}

std::string to_string(long value) {
    auto c = ::sprintf(buffer, "%ld", value);
    return std::string(buffer, buffer + c);
}

std::string to_string(float value) {
    auto c = ::sprintf(buffer, "%f", (double) value);
    return std::string(buffer, buffer + c);
}

std::string to_string(double value) {
    auto c = ::sprintf(buffer, "%f", value);
    return std::string(buffer, buffer + c);
}

std::string to_string(const Vec2& v) {
    auto c = ::sprintf(buffer, "(%f, %f)", v.x, v.y);
    return std::string(buffer, buffer + c);
}

std::string to_string(const Vec3& v) {
    auto c = ::sprintf(buffer, "(%f, %f, %f)", v.x, v.y, v.z);
    return std::string(buffer, buffer + c);
}

std::string to_string(const Vec4& v) {
    auto c = ::sprintf(buffer, "(%f, %f, %f, %f)", v.x, v.y, v.z, v.w);
    return std::string(buffer, buffer + c);
}


int stoi(const std::string& str) {
    return atoi(str.c_str());
}

float stof(const std::string& str) {
    return atof(str.c_str());
}

long stol(const std::string& str) {
    return atol(str.c_str());
}

}



