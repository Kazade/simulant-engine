#pragma once

#include "../generic/any/any.h"
#include "../generic/optional.h"
#include "limited_string.h"
#include <initializer_list>
#include <vector>

namespace smlt {

/* A set of named arguments that have been passed
   to construct a stage node. It's up to the stage node
   to define which args and types are valid */

typedef std::vector<LimitedString<32>> ConstructionArgNames;

class ConstructionArgs {
public:
    ConstructionArgs() = default;
    ConstructionArgs(
        std::initializer_list<std::pair<std::string, smlt::any>> params) {
        for(auto& p: params) {
            set_arg(p.first.c_str(), p.second);
        }
    }

    template<typename T>
    bool set_arg(const char* name, T value);
    bool has_arg(const char* name) const;

    template<typename T>
    optional<T> arg(const char* name) const;

    ConstructionArgNames arg_names() const;

    template<typename T>
    ConstructionArgs set(const char* name, T value) {
        auto ret = *this;
        ret.set_arg(name, value);
        return ret;
    }
};

} // namespace smlt
