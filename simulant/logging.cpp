#include <unordered_map>
#include <cassert>
#include <stdexcept>
#include "logging.h"

#ifdef __ANDROID__
#include <android/log.h>
#elif defined(__PSP__)
#include <pspdebug.h>
#endif

namespace smlt {

std::string to_string(const DateTime& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    return std::ctime(&t);
}

void Handler::write_message(Logger* logger,
                   const DateTime& time,
                   const std::string& level,
                   const std::string& message) {

    assert(logger);
    do_write_message(logger, time, level, message);
}

FileHandler::FileHandler(const std::string& filename):
    filename_(filename) {

    stream_.open(filename_.c_str());

    if(!stream_.good()) {
        throw std::runtime_error(_F("Unable to open log file at {0}").format(filename_));
    }
}

void FileHandler::do_write_message(Logger* logger,
                   const DateTime& time,
                   const std::string& level,
                   const std::string &message) {

    if(!stream_.good()) {
        throw std::runtime_error("Error writing to log file");
    }
    stream_ << to_string(time) << " " << level << " " << message << std::endl;
    stream_.flush();
}

StdIOHandler::StdIOHandler() {

}

void StdIOHandler::do_write_message(Logger* logger,
                                    const DateTime& time,
                       const std::string& level,
                       const std::string& message) {

    // We lock so that we don't get interleaved logging
    thread::Lock<thread::Mutex> g(lock_);

    if(level == "ERROR") {
#ifndef __ANDROID__
        std::cerr << to_string(time) << " ERROR " << message << std::endl;
#else
        __android_log_write(ANDROID_LOG_ERROR, "SIMULANT", message.c_str());
#endif
    } else {
#ifndef __ANDROID__
        std::cout << to_string(time) << " " << level << " " << message << std::endl;
#else
        __android_log_write(ANDROID_LOG_INFO, "SIMULANT", message.c_str());
#endif
    }
}

void debug(const std::string& text, const std::string& file, int32_t line) {
    get_logger("/")->debug(text, file, line);
}

void info(const std::string& text, const std::string& file, int32_t line) {
    get_logger("/")->info(text, file, line);
}

void warn(const std::string& text, const std::string& file, int32_t line) {
    get_logger("/")->warn(text, file, line);
}

void error(const std::string& text, const std::string& file, int32_t line) {
    get_logger("/")->error(text, file, line);
}

Logger* get_logger(const std::string& name) {
    typedef std::unordered_map<std::string, Logger::ptr> LoggerMap;

    /* Static deinitialization hack, destructors won't get called! */
    static Logger* root = new Logger("/");
    static LoggerMap* loggers_ = new LoggerMap();

    if(name.empty() || name == "/") {
        return root;
    } else {
        if(loggers_->find(name) == loggers_->end()) {
            loggers_->insert(std::make_pair(name, std::make_shared<Logger>(name)));
        }
        return loggers_->at(name).get();
    }
}

}
