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

#include <stdexcept>
#include "../logging.h"

#ifdef __DREAMCAST__
    #include "../../../deps/libgl/include/GL/gl.h"
    #include "../../../deps/libgl/include/GL/glext.h"
#elif defined(__PSP__)
    #include <GL/gl.h>
#else
    #include "../renderers/glad/glad/glad.h"
#endif

namespace smlt {
namespace GLChecker {

bool USE_GL_GET_ERROR = false;

void end_of_frame_check() {
    if(glGetError() != GL_NO_ERROR) {
        USE_GL_GET_ERROR = true;
    }
}

}

void check_and_log_error(const char *function_name) {
    GLuint error = glGetError();
    if(error != GL_NO_ERROR) {
        std::string error_string;
        switch(error) {
        case GL_INVALID_ENUM:
            error_string = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error_string = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error_string = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error_string = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error_string = "GL_OUT_OF_MEMORY";
            break;
#ifndef __ANDROID__
        case GL_STACK_OVERFLOW:
            error_string = "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error_string = "GL_STACK_UNDERFLOW";
            break;
#endif
        }

        S_ERROR("An OpenGL error occurred: {0} - {1}", function_name, error_string);

        throw std::runtime_error(_F("GL ERROR: {0}").format(error_string));
    }
}
}
