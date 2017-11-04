//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdexcept>
#include "../deps/kazlog/kazlog.h"

#ifdef _arch_dreamcast
#include <GL/gl.h>
#else

#ifdef SIMULANT_GL_VERSION_2X
    #include "../renderers/gl2x/glad/glad/glad.h"
#else
    #include "../renderers/gl1x/glad/glad/glad.h"
#endif

#endif

namespace GLChecker {

bool USE_GL_GET_ERROR = false;

void end_of_frame_check() {
    if(glGetError() != GL_NO_ERROR) {
        USE_GL_GET_ERROR = true;
    }
}

}

void check_and_log_error(const std::string &function_name) {
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
#ifndef _arch_dreamcast
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error_string = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
#endif
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

        L_ERROR(_F("An OpenGL error occurred: {0} - {1}").format(function_name, error_string));

        throw std::runtime_error(_F("GL ERROR: {0}").format(error_string));
    }
}
