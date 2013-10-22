#ifndef KGLT_GL_ERROR_H
#define KGLT_GL_ERROR_H

#include <string>
#include <functional>

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
    return GLChecker::Checker<Res, Func, Args...>::run(std::forward<Func>(func), std::forward<Args>(args)...);
}


#endif
