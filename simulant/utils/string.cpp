#include "string.h"

namespace smlt {

#if defined(_arch_dreamcast) || defined(ANDROID)
static float stof(const std::string& str) {
    const char* inp = str.c_str();
    char* p = nullptr;

#ifdef _arch_dreamcast
    // Dreamcast (GCC 4.7.3) doesn't define strtof but does define strtod
    float test = (float) strtod(inp, &p);
#else
    auto test = strtof(inp, &p);
#endif
    if(p == inp) {
        throw std::invalid_argument("Couldn't convert from a string to a float");
    }

    return (float) test;
}
#endif

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
