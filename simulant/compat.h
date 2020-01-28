#pragma once

/*
 * This header tries to patch missing bits of the STL on various
 * platforms. Specifically on the standard Dreamcast SDK the older
 * GCC compiler doesn't include things like std::to_string
 */

#include <cassert>
#include <string>
#include <exception>

#ifndef __clang__
// FIXME: these version checks may not be correct, I'm not sure
// which version improved the define checking
#if (__GNUC__ == 4 || __GNUC__ == 5 || __GNUC__ == 6 || __GNUC__ == 7)
#ifndef _GLIBCXX_USE_C99_STDIO

namespace std {

/* to_string stuff */
std::string to_string(int value);
std::string to_string(unsigned value);
std::string to_string(long value);
std::string to_string(float value);
std::string to_string(double value);

}

#endif
#endif
#endif
