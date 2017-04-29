/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

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
