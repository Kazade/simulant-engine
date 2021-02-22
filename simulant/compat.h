#pragma once

/*
 * This header tries to patch missing bits of the STL on various
 * platforms. Specifically on the standard Dreamcast SDK the older
 * GCC compiler doesn't include things like std::to_string
 */

#include <cassert>
#include <string>
#include <exception>


namespace std {

#if defined(__DREAMCAST__) || defined(__PSP__)
/* to_string stuff */
std::string to_string(int value);
std::string to_string(unsigned value);
std::string to_string(unsigned long value);
std::string to_string(long value);
std::string to_string(float value);
std::string to_string(double value);

int stoi(const std::string& str);
float stof(const std::string& str);
#endif

}
