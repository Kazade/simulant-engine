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

#ifndef GL_THREAD_CHECK_H
#define GL_THREAD_CHECK_H

#include <stdexcept>
#include <thread>
#include <memory>

#include "../deps/kazlog/kazlog.h"

class WrongThreadError:
    public std::runtime_error {

public:
    WrongThreadError():
        std::runtime_error("Attempted to call OpenGL from the wrong thread") {}
};

class GLThreadCheck;

extern std::shared_ptr<GLThreadCheck> GL_thread;

class GLThreadCheck {
public:
    static void init() {
        GL_thread.reset(new GLThreadCheck(std::this_thread::get_id()));
    }

    static void cleanup() {
        GL_thread.reset();
    }

    static void check();

    static bool is_current() {
        if(!GL_thread) {
            return false;
        }

        return GL_thread->do_check(false);
    }

private:
    GLThreadCheck(std::thread::id render_thread):
        render_thread_id_(render_thread) {}

    bool do_check(bool raise=true) {

        if(std::this_thread::get_id() != render_thread_id_) {
            if(raise) {
                throw WrongThreadError();
            } else {
                return false;
            }
        }

        return true;
    }


    std::thread::id render_thread_id_;
};

#endif // GL_THREAD_CHECK_H
