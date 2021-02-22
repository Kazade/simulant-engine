#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace smlt {

class Formatter {
public:
    Formatter(const char* fmt):
        fmt_(fmt) {}

    Formatter(const std::string& fmt):
        fmt_(fmt) {}

    template<typename... Args>
    std::string format(Args&& ...args) {
        return _format(fmt_, 0, std::forward<Args>(args)...);
    }

private:
    std::string _format(std::string str, int) {
        return str;
    }

    static bool is_int(char ch) {
        return ch >= 48 && ch < 58;
    }

    template<typename... Args>
    std::string _format(const std::string& str, int c, int8_t&& v, Args&& ...args) {
        return _format(str, c, (int16_t) v, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    std::string _format(const std::string& str, int c, T&& v, Args&& ...args) {
        std::stringstream ss;
        int max_token_size = 7;

        for(auto i = 0u; i < str.size(); ++i) {
            if(str[i] == '{') {
                std::string id, format;
                std::string* t = &id;
                uint8_t k = 0;
                for(auto j = 1; j < max_token_size; ++j) {
                    k = i + j;

                    if(k >= str.size() - 1) {
                        break;
                    }

                    char ch = str[k];

                    if(ch == '}') {
                        break;
                    } else if(ch == ':') {
                        t = &format;
                    } else {
                        if(!is_int(ch)) {
                            if(t == &id) {
                                // Id's only contain ints
                                break;
                            } else if(ch != '.') {
                                // Formatters can contain a .
                                break;
                            }
                        }

                        *t += str[k];
                    }
                }

                if(str[k] == '}') {
                    /* This was a placeholder within the size limit and
                     * with valid characters */
                    if(std::stoi(id) == c) {
                        /* OK we're dealing with this one */

                        if(format.size() > 1 && format[0] == '.') {
                            auto prec = std::stoi(format.substr(1));
                            ss << std::setprecision(prec) << v;
                        } else {
                            ss << v;
                        }

                        /* Skip past the placeholder */
                        i = k;
                    } else {
                        ss << str[i];
                    }
                } else {
                    ss << str[i];
                }
            } else {
                ss << str[i];
            }
        }

        return _format(ss.str(), c + 1, std::forward<Args>(args)...);
    }

    std::string fmt_;
};

}

typedef smlt::Formatter _F;

