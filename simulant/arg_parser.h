#pragma once

#include <map>
#include <vector>

#include "generic/any/any.h"
#include "generic/optional.h"
#include "utils/string.h"

namespace smlt {
enum ArgType {
    ARG_TYPE_BOOLEAN,
    ARG_TYPE_STRING,
    ARG_TYPE_INTEGER,
    ARG_TYPE_FLOAT
};

enum ArgCount {
    ARG_COUNT_ONE,
    ARG_COUNT_ONE_OR_MANY,
    ARG_COUNT_ZERO_OR_ONE,
    ARG_COUNT_ZERO_OR_MANY
};

class ArgParser {
public:
    typedef std::multimap<std::string, any> Args;

    void define_arg(
        const std::string& name,
        ArgType type,
        const std::string& help="",
        const std::string& var_name="",
        ArgCount count=ARG_COUNT_ONE
    );

    template<typename T>
    optional<T> arg_value(const std::string& name) {
        auto it = args_.find(name);
        if(it != args_.end()) {
            return optional<T>(any_cast<T>(it->second));
        } else {
            return optional<T>();
        }
    }

    template<typename T>
    std::vector<T> arg_value_list(const std::string& name) {
        std::vector<T> ret;

        auto it = args_.lower_bound(name);
        for(; it != args_.upper_bound(name); ++it) {
            ret.push_back(any_cast<T>(it->second));
        }

        return ret;
    }

    template<typename T>
    optional<T> arg_value(const std::string& name, T def) {
        auto it = args_.find(name);
        if(it != args_.end()) {
            return optional<T>(any_cast<T>(it->second));
        } else {
            return optional<T>((T) def);
        }
    }

    const Args& parsed_args() const {
        return args_;
    }

    void print_help() const;
    bool parse_args(int argc, char* argv[]);

private:
    struct DefinedArg {
        ArgType type;
        std::string var_name;
        std::string help;
        ArgCount count;
    };

    std::map<std::string, DefinedArg> defined_args_;
    Args args_;
};

}
