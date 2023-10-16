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
#include "utils/formatter.h"
#include "threads/mutex.h"
#include "threads/thread.h"

#include "compat.h"

namespace smlt {

enum LogLevel {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_VERBOSE = 5
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
public:
    StdIOHandler();

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

    void verbose(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_VERBOSE) return;

        write_message("VERBOSE", "\x1b[97m" + text + "\x1b[0m", file, line);
    }

    void debug(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_DEBUG) return;

        write_message("DEBUG", text, file, line);
    }

    void info(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_INFO) return;

        write_message("INFO", "\x1b[36m" + text + "\x1b[0m", file, line);
    }

    void warn(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_WARN) return;

        write_message("WARN", "\x1b[33m" + text + "\x1b[0m", file, line);
    }

    void error(const std::string& text, const std::string& file="None", int32_t line=-1) {
        if(level_ < LOG_LEVEL_ERROR) return;

        write_message("ERROR", "\x1b[31m" + text + "\x1b[0m", file, line);
    }

    void set_level(LogLevel level) {
        level_ = level;
    }

private:
    void write_message(const std::string& level, const std::string& text,
                       const std::string& file, int32_t line) {

        std::stringstream s;
        s << thread::this_thread_id() << ": ";

        if(line > -1) {
            s << text << " (" << file << ":" << line << ")";
        } else {
            s << text;
        }

        for(uint32_t i = 0; i < handlers_.size(); ++i) {
            handlers_[i]->write_message(this, std::chrono::system_clock::now(), level, s.str());
        }
    }

    std::string name_;
    std::vector<Handler::ptr> handlers_;

    LogLevel level_;
};

Logger* get_logger(const std::string& name);

void verbose(const std::string& text, const std::string& file="None", int32_t line=-1);
void debug(const std::string& text, const std::string& file="None", int32_t line=-1);
void info(const std::string& text, const std::string& file="None", int32_t line=-1);
void warn(const std::string& text, const std::string& file="None", int32_t line=-1);
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

#ifndef NDEBUG

#define S_VERBOSE(str, ...) \
    smlt::verbose(_F(str).format(__VA_ARGS__), __FILE__, __LINE__)

#define S_DEBUG(str, ...) \
    smlt::debug(_F(str).format(__VA_ARGS__), __FILE__, __LINE__)

#define S_INFO(str, ...) \
    smlt::info(_F(str).format(__VA_ARGS__), __FILE__, __LINE__)

#define S_WARN(str, ...) \
    smlt::warn(_F(str).format(__VA_ARGS__), __FILE__, __LINE__)

#define S_ERROR(str, ...) \
    smlt::error(_F(str).format(__VA_ARGS__), __FILE__, __LINE__)

#define S_DEBUG_ONCE(str, ...) \
    do { static char _done = 0; if(!_done++) smlt::debug(_F(str).format(__VA_ARGS__)); } while(0)

#define S_INFO_ONCE(str, ...) \
    do { static char _done = 0; if(!_done++) smlt::info(_F(str).format(__VA_ARGS__)); } while(0)

#define S_WARN_ONCE(str, ...) \
    do { static char _done = 0; if(!_done++) smlt::warn(_F(str).format(__VA_ARGS__)); } while(0)

#define S_ERROR_ONCE(str, ...) \
    do { static char _done = 0; if(!_done++) smlt::error(_F(str).format(__VA_ARGS__)); } while(0)

#else

#define S_VERBOSE(str, ...) \
    smlt::verbose(_F(str).format(__VA_ARGS__))

#define S_DEBUG(str, ...) \
    smlt::debug(_F(str).format(__VA_ARGS__))

#define S_INFO(str, ...) \
    smlt::info(_F(str).format(__VA_ARGS__))

#define S_WARN(str, ...) \
    smlt::warn(_F(str).format(__VA_ARGS__))

#define S_ERROR(str, ...) \
    smlt::error(_F(str).format(__VA_ARGS__))

#define S_DEBUG_ONCE(str, ...) \
    do { static char _done = 0; if(!_done++) smlt::debug(_F(str).format(__VA_ARGS__)); } while(0)

#define S_INFO_ONCE(str, ...) \
    do { static char _done = 0; if(!_done++) smlt::info(_F(str).format(__VA_ARGS__)); } while(0)

#define S_WARN_ONCE(str, ...) \
    do { static char _done = 0; if(!_done++) smlt::warn(_F(str).format(__VA_ARGS__)); } while(0)

#define S_ERROR_ONCE(str, ...) \
    do { static char _done = 0; if(!_done++) smlt::error(_F(str).format(__VA_ARGS__)); } while(0)

#endif
