#ifndef KGLT_GL_ERROR_H
#define KGLT_GL_ERROR_H

#include "glcompat.h"

#include <string>

#include <kazbase/exceptions.h>
#include <kazbase/unicode.h>
#include <kazbase/logging.h>
#include "gl_thread_check.h"
#include "../buffer_object.h"

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

class GLStateStash {
public:
    GLStateStash(GLenum state):
        state_(state),
        int_value_(0),
        boolean_value_(false) {

        switch(state) {
            case GL_ARRAY_BUFFER_BINDING:
            case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            case GL_CURRENT_PROGRAM:
            case GL_TEXTURE_BINDING_2D:
                GLCheck(glGetIntegerv, state, &int_value_);
            break;
            case GL_BLEND:
            case GL_DEPTH_TEST:
                GLCheck(glGetBooleanv, state, &boolean_value_);
            break;
        default:
            L_ERROR(_u("Unhandled state: {0}").format(state_));
            throw NotImplementedError(__FILE__, __LINE__);
        }
    }

    ~GLStateStash() {
        switch(state_) {
            case GL_ARRAY_BUFFER_BINDING:
            GLCheck(glBindBuffer, GL_ARRAY_BUFFER, int_value_);
            break;
            case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            GLCheck(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, int_value_);
            break;
            case GL_CURRENT_PROGRAM:
            GLCheck(glUseProgram, int_value_);
            break;
            case GL_TEXTURE_BINDING_2D:
            GLCheck(glBindTexture, GL_TEXTURE_2D, int_value_);
            break;
            case GL_BLEND:
            (boolean_value_) ? GLCheck(glEnable, GL_BLEND) : GLCheck(glDisable, GL_BLEND);
            break;
            case GL_DEPTH_TEST:
            (boolean_value_) ? GLCheck(glEnable, GL_DEPTH_TEST) : GLCheck(glDisable, GL_DEPTH_TEST);
            break;
        default:
            L_ERROR(_u("Unable to restore unhandled state: {0}").format(state_));
        }
    }

private:
    GLenum state_;
    GLint int_value_;
    GLboolean boolean_value_;
};

#endif
