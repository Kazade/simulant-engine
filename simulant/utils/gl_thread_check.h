/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GL_THREAD_CHECK_H
#define GL_THREAD_CHECK_H

#include <stdexcept>
#include <memory>

#include "../threads/thread.h"
#include "../logging.h"

namespace smlt {

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
        GL_thread.reset(new GLThreadCheck(thread::this_thread_id()));
    }

    static void clean_up() {
        GL_thread.reset();
    }

    static void check();

    static bool is_current() {
        if(!GL_thread) {
            return false;
        }

        return GL_thread->do_check(false);
    }

    thread::ThreadID thread_id() const {
        return render_thread_id_;
    }

private:
    GLThreadCheck(thread::ThreadID render_thread);

    bool do_check(bool raise=true) {

        if(thread::this_thread_id() != render_thread_id_) {
            if(raise) {
                throw WrongThreadError();
            } else {
                return false;
            }
        }

        return true;
    }


    thread::ThreadID render_thread_id_;
};

}

#endif // GL_THREAD_CHECK_H
