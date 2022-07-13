#pragma once

#include <vector>
#include <string>

namespace smlt {

bool starts_with(const std::string& s, const std::string& what);
bool ends_with(const std::string& s, const std::string& what);
bool contains(const std::string& s, const std::string& what);
std::size_t count(const std::string& s, const std::string& what);
std::vector<std::string> split(const std::string& s, const std::string& delim="", const int32_t count=-1);
std::string strip(const std::string& s, const std::string& what=" \t\n\r");
std::string replace_all(const std::string& s, const std::string& search, const std::string& replace);
std::string lower_case(const std::string& s);
std::string upper_case(const std::string& s);
}

