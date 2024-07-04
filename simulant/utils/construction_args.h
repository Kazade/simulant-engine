#pragma once

#include "../generic/any/any.h"
#include "../generic/optional.h"
#include "../logging.h"
#include "../utils/unicode.h"
#include "limited_string.h"
#include <initializer_list>
#include <map>
#include <vector>

namespace smlt {

/* A set of named arguments that have been passed
   to construct a stage node. It's up to the stage node
   to define which args and types are valid.

   Note: Due to unresolved problems with const char* and smlt::any
   passing a const char* value will implicitly convert to std::string
   internally and must be fetched using std::string.
*/

typedef std::vector<LimitedString<32>> ConstructionArgNames;

class Params {
public:
    Params() = default;
    Params(const std::initializer_list<smlt::any>& params) {
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
    bool set_arg(const char* name, T value) {
        auto existed = dict_.count(name);
        dict_[name] = value;
        return !existed;
    }

    bool set_arg(const char* name, any value) {
        auto existed = dict_.count(name);
        dict_.insert(std::make_pair(name, value));
        return !existed;
    }

    bool set_arg(const char* name, const char* value) {
        auto existed = dict_.count(name);
        dict_[name] = std::string(value);
        return !existed;
    }

    bool has_arg(const char* name) const {
        return dict_.count(name);
    }

    template<typename T>
    optional<T> arg(const char* name) const {
        auto it = dict_.find(name);
        if(it == dict_.end()) {
            return no_value;
        }

        return any_cast<T>(it->second);
    }

    ConstructionArgNames arg_names() const {
        ConstructionArgNames ret;
        for(auto& p: dict_) {
            ret.push_back(p.first);
        }
        return ret;
    }

    template<typename T>
    Params set(const char* name, T value) {
        S_ERROR("Setting {0} in {1}", typeid(T).name(), name);
        set_arg(name, value);
        return *this;
    }

    optional<any> raw(const char* name) const {
        auto it = dict_.find(name);
        if(it != dict_.end()) {
            return it->second;
        }

        return no_value;
    }

private:
    std::map<LimitedString<32>, any> dict_;
};

} // namespace smlt
