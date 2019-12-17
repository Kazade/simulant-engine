#pragma once

#include <memory>

#ifdef _arch_dreamcast
    #include "../../../deps/libgl/include/gl.h"
#else
    #include "../glad/glad/glad.h"
#endif

#include "../../material.h"

namespace smlt {

struct GL1RenderGroupImpl {
    GLuint texture_id[MAX_TEXTURE_UNITS] = {0};
};


}
