#pragma once

#include "../generic/any/any.h"
#include "../generic/optional.h"
#include "../logging.h"
#include "../utils/unicode.h"
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
    ConstructionArgs(const std::initializer_list<smlt::any>& params) {
        int i = 0;
        std::string key;
        bool skip = false;
        for(auto& param: params) {
            if(skip) {
                skip = false;
            } else {
                if(i % 2 == 0) {
                    try {
                        key = any_cast<const char*>(param);
                    } catch(bad_any_cast& e) {
                        try {
                            key = any_cast<std::string>(param);
                        } catch(bad_any_cast& e) {
                            try {
                                key = any_cast<unicode>(param).encode();
                            } catch(bad_any_cast& e) {
                                S_ERROR("Couldn't parse arg: {0}", i);
                                skip = true;
                            }
                        }
                    }
                } else {
                    set_arg(key.c_str(), param);
                }
            }

            ++i;
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
