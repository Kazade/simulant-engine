#ifndef __ANDROID__
	#include <GL/glew.h>
#else
	#include <GLES2/gl2.h>
#endif

#include <boost/format.hpp>

#include "../kazbase/logging.h"
#include "../kazbase/exceptions.h"
#include "../kazbase/unicode.h"

void check_and_log_error(std::string file, int lineno) {
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
        case GL_STACK_OVERFLOW:
            error_string = "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error_string = "GL_STACK_UNDERFLOW";
            break;
        }

        if(!file.empty()) {
            L_ERROR((boost::format("An OpenGL error occurred: %s:%d - %d") % file % lineno % error).str());
        } else {
            L_ERROR(_u("An OpenGL error occurred: {0}").format(error));
        }

        throw RuntimeError(_u("GL ERROR: {0}").format(error_string).encode());
    }
}

