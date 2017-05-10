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

#ifdef SIMULANT_GL_VERSION_1X


#include "gl1x_renderer.h"
#ifdef _arch_dreamcast
    #include <GL/gl.h>
#else
    #include "./glad/glad/glad.h"
#endif

#include "../../utils/gl_error.h"
#include "../../material.h"
#include "../../material_constants.h"
#include "../../texture.h"

namespace smlt {

class GL1RenderGroupImpl: public batcher::RenderGroupImpl {
public:
    GL1RenderGroupImpl(RenderPriority priority):
        batcher::RenderGroupImpl(priority) {}

    GLuint texture_id[MAX_TEXTURE_UNITS] = {0};

    bool lt(const RenderGroupImpl& other) const override {
        const GL1RenderGroupImpl* rhs = dynamic_cast<const GL1RenderGroupImpl*>(&other);
        if(!rhs) {
            // Should never happen... throw an error maybe?
            return false;
        }

        for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            if(texture_id[i] < rhs->texture_id[i]) {
                return true;
            } else if(texture_id[i] > rhs->texture_id[i]) {
                return false;
            }
        }

        return false;
    }
};

batcher::RenderGroup GL1XRenderer::new_render_group(Renderable *renderable, MaterialPass *material_pass) {
    auto impl = std::make_shared<GL1RenderGroupImpl>(renderable->render_priority());

    for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        if(i < material_pass->texture_unit_count()) {
            auto tex_id = material_pass->texture_unit(i).texture_id().fetch()->gl_tex();
            impl->texture_id[i] = tex_id;
        } else {
            impl->texture_id[i] = 0;
        }
    }

    return batcher::RenderGroup(impl);
}

void GL1XRenderer::init_context() {
#ifndef _arch_dreamcast
    if(!gladLoadGL()) {
        throw std::runtime_error("Unable to intialize OpenGL 1.1");
    }
#endif

    GLCheck(glEnable, GL_DEPTH_TEST);
    GLCheck(glDepthFunc, GL_LEQUAL);
    GLCheck(glEnable, GL_CULL_FACE);
}


}

#endif // SIMULANT_GL_VERSION_1X
