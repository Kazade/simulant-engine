#pragma once

#include <memory>

#include "../gl_includes.h"
#include "../../assets/material.h"
#include "../gl_renderer.h"

namespace smlt {

struct GL1RenderGroupImpl {
    GLuint texture_id[_S_GL_MAX_TEXTURE_UNITS] = {0};
};


}
