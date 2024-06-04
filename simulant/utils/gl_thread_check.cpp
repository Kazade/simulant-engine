//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../logging.h"
#include "gl_thread_check.h"

namespace smlt {
std::shared_ptr<GLThreadCheck> GL_thread;

void GLThreadCheck::check() {
    try {
        if(GL_thread) {
            GL_thread->do_check();
        }
    } catch(WrongThreadError& e) {
        _S_UNUSED(e);
        S_ERROR(
            "Tried to call OpenGL dependent code from the wrong thread {0} vs {1}",
            GL_thread->thread_id(), thread::this_thread_id()
        );
        throw;
    }
}

GLThreadCheck::GLThreadCheck(thread::ThreadID render_thread):
    render_thread_id_(render_thread) {

}

}
