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

        glGetIntegerv(state, &original_value_);
    }

    ~GLStateStash() {
        switch(state_) {
            case GL_VERTEX_ARRAY_BINDING:
            GLCheck(glBindVertexArray, original_value_);
            break;
        default:
            L_ERROR(_u("Unhandled state: {0}").format(state_));
            throw NotImplementedError(__FILE__, __LINE__);
        }
    }

private:
    GLenum state_;
    GLint original_value_;
};

#endif
