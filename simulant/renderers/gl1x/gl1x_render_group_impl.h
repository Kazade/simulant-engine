#pragma once

#include <memory>

#ifdef _arch_dreamcast
    #include "../../../deps/libgl/include/gl.h"
#elif defined(__PSP__)
    #include <GL/gl.h>
#else
    #include "../glad/glad/glad.h"
#endif

#include "../../material.h"
#include "../gl_renderer.h"

namespace smlt {

struct GL1RenderGroupImpl {
    GLuint texture_id[_S_GL_MAX_TEXTURE_UNITS] = {0};
};


}
