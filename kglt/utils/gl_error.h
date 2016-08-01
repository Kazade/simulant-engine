#ifndef KGLT_GL_ERROR_H
#define KGLT_GL_ERROR_H

#include <string>

#include <kazbase/exceptions.h>
#include <kazbase/unicode.h>
#include <kazbase/logging.h>
#include "gl_thread_check.h"

void check_and_log_error(const std::string& function_name);

namespace GLChecker {

/*
 * glGetError is ridiculously slow, so what we do is call it once per frame,
 * if that call returns an error, then we enable it for every call and die
 * when we get an error on the next frame. Make sense?
 */

extern bool USE_GL_GET_ERROR;

void end_of_frame_check();

template<typename Res, typename Func, typename... Args>
struct Checker {
    static Res run(const std::string& function_name, Func&& func, Args&&... args) {
        Res result = func(std::forward<Args>(args)...);
        if(USE_GL_GET_ERROR) {
            check_and_log_error(function_name);
        }
        return result;
    }
};

template<typename Func, typename... Args>
struct Checker<void, Func, Args...> {
    static void run(const std::string& function_name, Func&& func, Args&&... args) {
        func(std::forward<Args>(args)...);
        if(USE_GL_GET_ERROR) {
            check_and_log_error(function_name);
        }
    }
};

template<typename Func>
struct Checker<void, Func> {
    static void run(const std::string& function_name, Func&& func) {
        func();
        if(USE_GL_GET_ERROR) {
            check_and_log_error(function_name);
        }
    }
};

}

template<typename Res=void, typename Func, typename... Args>
Res _GLCheck(const std::string& function_name, Func&& func, Args&&... args) {
    GLThreadCheck::check();
    return GLChecker::Checker<Res, Func, Args...>::run(function_name, std::forward<Func>(func), std::forward<Args>(args)...);
}

#ifndef GLCheck
#define GLCheck(...) _GLCheck(__func__, __VA_ARGS__)
#endif

#endif
