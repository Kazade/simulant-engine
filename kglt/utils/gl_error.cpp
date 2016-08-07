#include <kazbase/logging.h>
#include <kazbase/exceptions.h>
#include <kazbase/unicode.h>

#ifdef KGLT_GL_VERSION_2X
    #include "../renderers/gl2x/glad/glad/glad.h"
#else
    #include "../renderers/gl1x/glad/glad/glad.h"
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
        unicode error_string;
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

        L_ERROR(_u("An OpenGL error occurred: {0} - {1}").format(function_name, error_string));

        throw RuntimeError(_u("GL ERROR: {0}").format(error_string).encode());
    }
}
