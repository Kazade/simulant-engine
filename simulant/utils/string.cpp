#include "string.h"

#ifdef _arch_dreamcast

#include <cstdlib>
#include <stdexcept>
#include <climits>

namespace std {
float stof(const std::string& str) {
    const char* inp = str.c_str();
    char* p = nullptr;

    // Dreamcast (GCC 4.7.3) doesn't define strtof but does define strtod
    float test = (float) strtod(inp, &p);

    if(p == inp) {
        throw std::invalid_argument("Couldn't convert from a string to a float");
    }

    return (float) test;
}

int32_t stoi(const std::string& str) {
    const char* inp = str.c_str();
    char* p = nullptr;

    auto test = strtol(inp, &p, 10);

    if(p == inp || test == LONG_MIN || test == LONG_MAX) {
        throw std::invalid_argument("Couldn't convert from a string to an integer");
    }

    return (int32_t) test;
}

}
#endif

namespace smlt {

bool ends_with(const std::string& s, const std::string& what) {
    if(what.size() > s.size()) {
        return false;
    }

    return std::string(s.end() - what.size(), s.end()) == what;
}

bool contains(const std::string& s, const std::string& what) {
    return s.find(what) != std::string::npos;
}

std::size_t count(const std::string& s, const std::string& what) {
    std::size_t i = 0;
    std::size_t count = 0;
    while((i = s.find(what, i)) != std::string::npos) {
        ++count;
        ++i;
    }

    return count;
}

std::vector<std::string> split(const std::string& s, const std::string& delim, const int32_t count) {
    std::vector<std::string> result;

    std::string d = (delim.empty()) ? "\t\n\r " : delim;
    std::string buff;
    auto counter = 0;

    for(auto c: s) {
        if(d.find(c) != std::string::npos) {
            if(!buff.empty() ) {
                if(counter != count) {
                    result.push_back(buff);
                    buff = "";
                    ++counter;
                } else {
                    buff.push_back(c);
                }
            }
        } else {
            buff.push_back(c);
        }
    }

    if(!buff.empty()) {
        result.push_back(buff);
    }

    return result;
}

std::string strip(const std::string& s, const std::string& what) {
    int32_t i = 0;
    int32_t j = s.size() - 1;

    while(what.find(s[i]) != std::string::npos && i < (int32_t) s.size()) {++i;}
    while(what.find(s[j]) != std::string::npos && j >= 0) {--j;}

    if(j < i) {
        return "";
    }

    return std::string(s.begin() + i, s.begin() + (j + 1));
}

}
