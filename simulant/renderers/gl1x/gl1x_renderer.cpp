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

#include "gl1x_renderer.h"
#ifdef _arch_dreamcast
    #include "../../../deps/libgl/include/gl.h"
#else
    #include "../glad/glad/glad.h"
#endif

#include "../../utils/gl_error.h"
#include "../../material.h"
#include "../../material_constants.h"
#include "../../texture.h"
#include "gl1x_render_queue_visitor.h"
#include "gl1x_render_group_impl.h"
#include "gl1x_buffer_manager.h"

namespace smlt {

batcher::RenderGroup GL1XRenderer::new_render_group(
        Renderable *renderable, MaterialPass *material_pass,
        RenderPriority priority, bool is_blended, float distance_to_camera) {

    auto impl = std::make_shared<GL1RenderGroupImpl>(priority, is_blended, distance_to_camera);

    uint8_t used_count = 0;

    if(!material_pass->is_texturing_enabled()) {
        for(uint8_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            impl->texture_id[i] = 0;
        }
    } else {
        if(material_pass->diffuse_map().texture_id) {
            impl->texture_id[used_count++] = texture_objects_.at(
                material_pass->diffuse_map().texture_id
            );
        }

        if(material_pass->light_map().texture_id) {
            impl->texture_id[used_count++] = texture_objects_.at(
                material_pass->light_map().texture_id
            );
        }

        if(material_pass->normal_map().texture_id) {
            impl->texture_id[used_count++] = texture_objects_.at(
                material_pass->normal_map().texture_id
            );
        }

        if(material_pass->specular_map().texture_id) {
            impl->texture_id[used_count++] = texture_objects_.at(
                material_pass->specular_map().texture_id
            );
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

std::shared_ptr<batcher::RenderQueueVisitor> GL1XRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<GL1RenderQueueVisitor>(this, camera);
}

smlt::GL1XRenderer::GL1XRenderer(smlt::Window *window):
    Renderer(window),
    GLRenderer(window) {
}

}


