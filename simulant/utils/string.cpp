#include <algorithm>
#include "string.h"

namespace smlt {

bool starts_with(const std::string& s, const std::string& what) {
    return s.find(what) == 0;
}

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

    while(i < (int32_t) s.size() && what.find(s[i]) != std::string::npos) {++i;}
    while(j >= 0 && what.find(s[j]) != std::string::npos) {--j;}

    if(j < i) {
        return "";
    }

    return std::string(s.begin() + i, s.begin() + (j + 1));
}

std::string replace_all(const std::string &s, const std::string &search, const std::string &replace) {
    std::string result = s;

    auto pos = result.find(search);

    while( pos != std::string::npos) {
        result.replace(pos, search.size(), replace);
        pos = result.find(search, pos + replace.size());
    }

    return result;
}

std::string lower_case(const std::string& s) {
    std::string cpy = s;
    std::transform(cpy.begin(), cpy.end(), cpy.begin(), ::tolower);
    return cpy;
}

std::string upper_case(const std::string& s) {
    std::string cpy = s;
    std::transform(cpy.begin(), cpy.end(), cpy.begin(), ::toupper);
    return cpy;
}

}
