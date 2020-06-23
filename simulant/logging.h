#pragma once

#include <chrono>
#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <iomanip>

#include "utils/string.h"
#include "threads/mutex.h"
#include "threads/thread.h"

#include "compat.h"

namespace smlt {

enum LogLevel {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4
};


class Formatter {
private:

    struct Counter {
        Counter(int32_t val=0): val(val) {}
        int32_t val;
    };

public:
    Formatter(const std::string& format_string):
        str_(format_string) {

    }

    template<typename T>
    std::string format(T&& arg) {
        return do_format(Counter(), std::forward<T>(arg));
    }

    std::string format(int8_t&& arg) {
        return do_format(Counter(), (int16_t) arg);
    }

    template<typename... Args>
    std::string format(Args&& ... args) {
        return do_format(Counter(), std::forward<Args>(args)...);
    }

private:
    std::string str_;

    struct TokenFound {
        std::string group;
        std::string index;
        std::string remainder;
    };

    std::vector<TokenFound> scan(int c, const std::string& s) {
        std::vector<TokenFound> result;
        auto cs = std::to_string(c);
        auto text = "{" + cs;
        auto it = s.find(text);
        while(it != std::string::npos) {
            auto end = s.find("}", it);
            if(end == std::string::npos) {
                break;
            }

            TokenFound found;
            found.index = cs;
            found.group = s.substr(it, (end - it) + 1);

            auto sep = s.find(":", it);
            if(sep != std::string::npos) {
                found.remainder = s.substr(sep + 1, end + 1);
            }

            result.push_back(found);

            it = s.find(text, it + found.group.size());
        }
        return result;
    }

    /* Specialize so that an integer isn't treated like an ascii character..
     * (Why isn't int8_t its own type instead of `signed char` ??!)*/
    std::string do_format(Counter count, int8_t&& val) {
        int16_t c = val;
        return do_format(count, std::move(c));
    }

    template<typename T>
    std::string do_format(Counter count, T&& val) {
        std::string to_replace = "{" + std::to_string(count.val) + "}";

        std::stringstream ss;

        std::string result = str_;
        auto found = scan(count.val, result);

        for(auto& tok: found) {
            if(!tok.remainder.empty()) {
                // Something else is going on!
                if(tok.remainder[0] == '.') {
                    // Precision (e.g. {0:.2})

                    auto p = std::stoi(tok.remainder.substr(1));
                    ss << std::setprecision(p) << val;
                }
            } else {
                ss << val;
            }

            std::string replacement = ss.str();

            result = replace_all(
                result, tok.group, replacement
            );
        }

        return result;
    }

    template<typename T, typename... Args>
    std::string do_format(Counter count, T&& val, Args&&... args) {
        return Formatter(
            do_format(count, std::forward<T>(val))
        ).do_format(Counter(count.val + 1), std::forward<Args>(args)...);
    }
};

class Logger;

typedef std::chrono::time_point<std::chrono::system_clock> DateTime;

class Handler {
public:
    typedef std::shared_ptr<Handler> ptr;

    virtual ~Handler() {}
    void write_message(Logger* logger,
                       const DateTime& time,
                       const std::string& level,
                       const std::string& message);

private:
    virtual void do_write_message(Logger* logger,
                       const DateTime& time,
                       const std::string& level,
                       const std::string& message) = 0;
};

class StdIOHandler : public Handler {
private:
    void do_write_message(Logger* logger,
                       const DateTime& time,
                       const std::string& level,
                       const std::string& message) override;

    thread::Mutex lock_;
};

class FileHandler : public Handler {
public:
    FileHandler(const std::string& filename);

private:
    void do_write_message(Logger* logger,
                       const DateTime& time,
                       const std::string& level,
                       const std::string& message);
    std::string filename_;
    std::ofstream stream_;
};

class Logger {
public:
    typedef std::shared_ptr<Logger> ptr;

    /* WARNING: Do not add a destructor - it won't get called! */

    Logger(const std::string& name):
        name_(name),
        level_(LOG_LEVEL_DEBUG) {

    }

    void add_handler(Handler::ptr handler) {
        //FIXME: check it doesn't exist already
        handlers_.push_back(handler);
    }

    void debug(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_DEBUG) return;

        write_message("DEBUG", text, file, line);
    }

    void info(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_INFO) return;

        write_message("INFO", text, file, line);
    }

    void warn(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_WARN) return;

        write_message("WARN", text, file, line);
    }

    void warn_once(const std::string& text, const std::string& file="None", int32_t line=-1) {
        /*
         *  This is *slow*, be aware of that, don't call in performance critical code!
         */

        if(line == -1) {
            warn(text, file, line); //Can't warn once if no line is specified
            return;
        }

        static std::unordered_map<std::string, std::unordered_set<int32_t>> warned;

        bool already_logged = warned.find(file) != warned.end() && warned[file].count(line);

        if(already_logged) {
            return;
        } else {
            warned[file].insert(line);
            warn(text, file, line);
        }
    }

    void error(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_ERROR) return;

        write_message("ERROR", text, file, line);
    }

    void set_level(LogLevel level) {
        level_ = level;
    }

private:
    void write_message(const std::string& level, const std::string& text,
                       const std::string& file, int32_t line) {

        std::stringstream s;
        s << thread::this_thread_id() << ": ";
        s << text << " (" << file << ":" << Formatter("{0}").format(line) << ")";
        for(uint32_t i = 0; i < handlers_.size(); ++i) {
            handlers_[i]->write_message(this, std::chrono::system_clock::now(), level, s.str());
        }
    }

    std::string name_;
    std::vector<Handler::ptr> handlers_;

    LogLevel level_;
};

Logger* get_logger(const std::string& name);

void debug(const std::string& text, const std::string& file="None", int32_t line=-1);
void info(const std::string& text, const std::string& file="None", int32_t line=-1);
void warn(const std::string& text, const std::string& file="None", int32_t line=-1);
void warn_once(const std::string& text, const std::string& file="None", int32_t line=-1);
void error(const std::string& text, const std::string& file="None", int32_t line=-1);


class DebugScopedLog {
public:
    DebugScopedLog(const std::string& text, const std::string& file, uint32_t line):
        text_(text) {

        debug(smlt::Formatter("Enter: {0} ({1}, {2})").format(text, file, line));
    }

    ~DebugScopedLog() {
        debug(smlt::Formatter("Exit: {0}").format(text_));
    }

private:
    std::string text_;
};

}

typedef smlt::Formatter _F;

#define L_DEBUG(txt) \
    smlt::debug((txt), __FILE__, __LINE__)

#define L_INFO(txt) \
    smlt::info((txt), __FILE__, __LINE__)

#define L_WARN(txt) \
    smlt::warn((txt), __FILE__, __LINE__)

#define L_WARN_ONCE(txt) \
    smlt::warn_once((txt), __FILE__, __LINE__)

#define L_ERROR(txt) \
    smlt::error((txt), __FILE__, __LINE__)

#define L_DEBUG_N(name, txt) \
    smlt::get_logger((name))->debug((txt), __FILE__, __LINE__)

#define L_INFO_N(name, txt) \
    smlt::get_logger((name))->info((txt), __FILE__, __LINE__)

#define L_WARN_N(name, txt) \
    smlt::get_logger((name))->warn((txt), __FILE__, __LINE__)

#define L_ERROR_N(name, txt) \
    smlt::get_logger((name))->error((txt), __FILE__, __LINE__)

