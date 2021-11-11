#include <iomanip>
#include "logging.h"
#ifndef _WIN32
#include "deps/kfs/kfs.h"
#else
#include "deps/kfs/kfs/kfs.h"
#endif

#include "arg_parser.h"

namespace smlt {

void ArgParser::define_arg(const std::string& name, ArgType type, const std::string& help, const std::string& var_name, ArgCount count) {

    if(defined_args_.count(name)) {
        // FIXME: Warning?
        return;
    }

    if(name[0] != '-' || name[1] != '-') {
        S_ERROR("Args must begin with a double hyphen (e.g. --name)");
        return;
    }

    DefinedArg arg;
    arg.var_name = (var_name.empty()) ? name.substr(2) : var_name;
    arg.type = type;
    arg.count = count;
    arg.help = help;

    defined_args_.insert(std::make_pair(name, arg));
}

void ArgParser::print_help() const {
    auto exe = kfs::path::split(kfs::exe_path()).second;
    std::cout << "Usage: " << exe << " [OPTION]..." << std::endl << std::endl;

    for(auto& arg: defined_args_) {
        std::cout << std::left << std::setw(30) << "\t" + arg.first;
        std::cout << arg.second.help << std::endl;

    }

    std::cout << std::endl;
}

bool ArgParser::parse_args(int argc, char* argv[]) {
    /* Valid things:
         *
         * --var=X
         * --var X
         * --var=X --var=Y
         * --var X --var Y
         */

    std::vector<std::string> params(argv, argv + argc);

    std::string current_var;
    for(auto i = 0u; i < params.size(); ++i) {
        auto param = params[i];
        std::string next = (i < params.size() - 1) ? params[i + 1] : "";

        if(param.size() > 2 && param[0] == '-' && param[1] == '-') {
            current_var = param;

            std::string value;

            auto eq = current_var.find("=");

            if(eq != std::string::npos) {
                // Equality syntax
                value = current_var.substr(eq + 1);
                current_var = current_var.substr(0, eq);
            }

            auto it = defined_args_.find(current_var);
            if(it == defined_args_.end()) {
                S_ERROR("Unrecognized option: {0}", param);
                return false;
            }

            if(!value.empty()) {
                // Fall through to value handling below
                param = value;
            } else {
                // Handle --help type boolean flags
                if(it->second.type == ARG_TYPE_BOOLEAN) {
                    if(next.empty() || (next.size() > 2 && next[0] == '-' && next[1] == '-')) {
                        // No value in the next arg, so just assume true
                        // and fall through
                        param = "true";
                    }
                } else {
                    // No value, so go around again
                    continue;
                }
            }
        }

        if(current_var.empty()) {
            continue;
        }

        auto& info = defined_args_.at(current_var);

        if(info.count == ARG_COUNT_ONE && args_.count(info.var_name)) {
            // Too many args
            S_ERROR("Received more than one value for: {0}", current_var);
        } else if(info.count == ARG_COUNT_ZERO_OR_ONE && args_.count(info.var_name) == 1) {
            // Too many args
            S_ERROR("Received more than one value for: {0}", current_var);
        }

        if(info.type == ARG_TYPE_BOOLEAN) {
            args_.insert(std::make_pair(
                             info.var_name, (param == "1" || param == "true" || param == "yes" || param == "y" || param == "Y")
                             ));
        } else if(info.type == ARG_TYPE_FLOAT) {
            args_.insert(std::make_pair(
                             info.var_name, smlt::stof(param)
                             ));
        } else if(info.type == ARG_TYPE_INTEGER) {
            args_.insert(std::make_pair(
                             info.var_name, smlt::stoi(param)
                             ));
        } else {
            args_.insert(std::make_pair(info.var_name, param));
        }
    }

    return true;
}

}
