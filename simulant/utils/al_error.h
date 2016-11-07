#ifndef AL_ERROR_H
#define AL_ERROR_H

#include <string>
#include <functional>

namespace ALChecker {

void al_check_and_log_error();

template<typename Res, typename Func, typename... Args>
struct Checker {
    static Res run(Func&& func, Args&&... args) {
        Res result = std::bind(std::forward<Func>(func), std::forward<Args>(args)...)();
        al_check_and_log_error();
        return result;
    }
};

template<typename Func, typename... Args>
struct Checker<void, Func, Args...> {
    static void run(Func&& func, Args&&... args) {
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...)();
        al_check_and_log_error();
    }
};

template<typename Func>
struct Checker<void, Func> {
    static void run(Func&& func) {
        func();
        al_check_and_log_error();
    }
};

}

template<typename Res=void, typename Func, typename... Args>
Res ALCheck(Func&& func, Args&&... args) {
    return ALChecker::Checker<Res, Func, Args...>::run(std::forward<Func>(func), std::forward<Args>(args)...);
}

#endif // AL_ERROR_H
