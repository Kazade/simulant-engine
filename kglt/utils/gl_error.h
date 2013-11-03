#ifndef KGLT_GL_ERROR_H
#define KGLT_GL_ERROR_H

#include <string>
#include <functional>

#include "../kazbase/exceptions.h"
#include "../kazbase/unicode.h"
#include "../kazbase/logging.h"
#include "gl_thread_check.h"

void check_and_log_error(std::string file, int lineno);

namespace GLChecker {

template<typename Res, typename Func, typename... Args>
struct Checker {
    static Res run(Func&& func, Args&&... args) {
        Res result = std::bind(std::forward<Func>(func), std::forward<Args>(args)...)();
        check_and_log_error("", 0);
        return result;
    }
};

template<typename Func, typename... Args>
struct Checker<void, Func, Args...> {
    static void run(Func&& func, Args&&... args) {
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...)();
        check_and_log_error("", 0);
    }
};

template<typename Func>
struct Checker<void, Func> {
    static void run(Func&& func) {
        func();
        check_and_log_error("", 0);
    }
};

}

template<typename Res=void, typename Func, typename... Args>
Res GLCheck(Func&& func, Args&&... args) {
    GLThreadCheck::check();
    return GLChecker::Checker<Res, Func, Args...>::run(std::forward<Func>(func), std::forward<Args>(args)...);
}

class GLStateStash {
public:
    GLStateStash(GLenum state):
        state_(state) {

        switch(state) {
            case GL_VERTEX_ARRAY_BINDING:
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
        }


    }

    ~GLStateStash() {
        switch(state_) {
            case GL_VERTEX_ARRAY_BINDING:
            GLCheck(glBindVertexArray, int_value_);
            break;
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
            L_ERROR(_u("Unhandled state: {0}").format(state_));
            throw NotImplementedError(__FILE__, __LINE__);
        }
    }

private:
    GLenum state_;
    GLint int_value_;
    GLboolean boolean_value_;
};

#endif
