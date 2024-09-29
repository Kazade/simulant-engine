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
#ifdef __DREAMCAST__
    #include "../../../deps/libgl/include/GL/gl.h"
#elif defined(__PSP__)
    #include <GL/gl.h>
#else
    #include "../glad/glad/glad.h"
#endif

#include "../../assets/material.h"
#include "../../material_constants.h"
#include "../../meshes/vertex_buffer.h"
#include "../../texture.h"
#include "../../utils/gl_error.h"
#include "../../vertex_data.h"
#include "gl1x_render_group_impl.h"
#include "gl1x_render_queue_visitor.h"
#include "gl1x_vertex_buffer_data.h"

namespace smlt {

batcher::RenderGroupKey GL1XRenderer::prepare_render_group(
    batcher::RenderGroup* group,
    const Renderable *renderable,
    const MaterialPass *material_pass,
    const uint8_t pass_number,
    const bool is_blended,
    const float distance_to_camera) {

    _S_UNUSED(renderable);
    _S_UNUSED(material_pass);
    _S_UNUSED(group);

    return batcher::generate_render_group_key(
        pass_number,
        is_blended,
        distance_to_camera,
        renderable->precedence
    );
}

void GL1XRenderer::init_context() {
#ifndef __DREAMCAST__
#ifndef __PSP__
    if(!gladLoadGL()) {
        throw std::runtime_error("Unable to intialize OpenGL 1.1");
    }
#endif
#endif

    GLRenderer::init_context();

    const GLubyte* GL_vendor = glGetString(GL_VENDOR);
    const GLubyte* GL_renderer = glGetString(GL_RENDERER);
    const GLubyte* GL_version = glGetString(GL_VERSION);
    const GLubyte* GL_extensions = glGetString(GL_EXTENSIONS);

    S_INFO(
        "\n\nOpenGL Information:\n\n"
        "\tVendor: {0}\n"
        "\tRenderer: {1}\n"
        "\tVersion: {2}\n\n"
        "\tExtensions: {3}\n\n",
        GL_vendor, GL_renderer, GL_version, GL_extensions
    );

    GLCheck(glEnable, GL_DEPTH_TEST);
    GLCheck(glDepthFunc, GL_LEQUAL);
    GLCheck(glEnable, GL_CULL_FACE);
    GLCheck(glEnable, GL_MULTISAMPLE);
#ifndef __PSP__
    GLCheck(glHint, GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
#endif
}

std::shared_ptr<batcher::RenderQueueVisitor> GL1XRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<GL1RenderQueueVisitor>(this, camera);
}

smlt::GL1XRenderer::GL1XRenderer(smlt::Window *window):
    GLRenderer(window) {
}

std::shared_ptr<VertexBuffer>
    GL1XRenderer::prepare_vertex_data(const VertexData* vertex_data) {

    std::shared_ptr<GL1XVertexBufferData> vbuffer_data;
    VertexBufferPtr vertex_buffer = vertex_data->gpu_buffer();

    // If we already have a vertex buffer, then reuse it (save realloc)
    if(vertex_buffer) {
        auto renderer_data = vertex_buffer->renderer_data();
        vbuffer_data = std::dynamic_pointer_cast<GL1XVertexBufferData>(renderer_data);
    } else {
        vbuffer_data = std::make_shared<GL1XVertexBufferData>();
        vertex_buffer =
            vertex_buffer_factory(GL1XVertexBufferData::format, vbuffer_data);
    }

    auto vertex_count = vertex_data->count();

    assert(vertex_count);

    vbuffer_data->vertices.resize(vertex_count);
    for(uint32_t i = 0; i < vertex_count; ++i) {
        vbuffer_data->vertices[i].xyz =
            vertex_data->attr_as<Vec3>(VERTEX_ATTR_NAME_POSITION, i).value();
        vbuffer_data->vertices[i].uv =
            vertex_data->attr_as<Vec2>(VERTEX_ATTR_NAME_TEXCOORD_0, i)
                .value_or(Vec2());
        vbuffer_data->vertices[i].n =
            vertex_data->attr_as<Vec3>(VERTEX_ATTR_NAME_NORMAL, i)
                .value_or(Vec3());
        vbuffer_data->vertices[i].color =
            vertex_data->attr_as<Color>(VERTEX_ATTR_NAME_COLOR, i)
                .value_or(Color::white());
    }

    return vertex_buffer;
}
}


