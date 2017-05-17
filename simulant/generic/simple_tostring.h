#pragma once

#ifndef __clang__
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 7)

/* GCC 4.7 doesn't define std::to_string... for some reason, so we just hack around it here */

#include <string>
#include <cstdint>

namespace std {

std::string to_string(int value);
std::string to_string(unsigned value);
std::string to_string(long value);
std::string to_string(float value);
std::string to_string(double value);

}

#endif
#endif
