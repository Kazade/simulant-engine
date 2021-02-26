#pragma once

/*
 * This header tries to patch missing bits of the STL on various
 * platforms. Specifically on the standard Dreamcast SDK the older
 * GCC compiler doesn't include things like smlt::to_string
 */

#include <cassert>
#include <string>
#include <exception>


namespace smlt {

class Vec2;
class Vec3;
class Vec4;

/* to_string stuff */
std::string to_string(int value);
std::string to_string(unsigned value);
std::string to_string(unsigned long value);
std::string to_string(long value);
std::string to_string(float value);
std::string to_string(double value);

std::string to_string(const Vec2& v);
std::string to_string(const Vec3& v);
std::string to_string(const Vec4& v);

int stoi(const std::string& str);
float stof(const std::string& str);
long stol(const std::string& str);

}
