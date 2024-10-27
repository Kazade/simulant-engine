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

#include "../../application.h"
#include "../../asset_manager.h"
#include "../../assets/material.h"
#include "../../material_constants.h"
#include "../../meshes/vertex_buffer.h"
#include "../../texture.h"
#include "../../utils/gl_error.h"
#include "../../utils/mesh/triangulate.h"
#include "../../vertex_data.h"
#include "../../window.h"

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

void GL1XRenderer::on_pre_render() {
    GLRenderer::on_pre_render();

#ifndef __DREAMCAST__
    if(!normalization_cube_map_) {
        // The Dreamcast has a different way of dealing with
        // normal maps; and it's not exposed through GLdc so we
        // don't need to waste memory with a normalization cube map
        // there.
        init_normalization_map();
    }
#endif
}

void GL1XRenderer::init_normalization_map() {
    int size = 32;
    float offset = 0.5f;
    float half_size = ((float)size) * 0.5f;

    normalization_cube_map_ = get_app()->shared_assets->create_texture(
        size, size, TEXTURE_FORMAT_RGB_3UB_888, TEXTURE_TARGET_CUBE_MAP);

    normalization_cube_map_->set_texture_filter(TEXTURE_FILTER_POINT);

    for(int i = 0; i < 6; ++i) {
        normalization_cube_map_->mutate_data(
            [=](uint8_t* data, uint16_t w, uint16_t h, TextureFormat) {
            int j = 0;
            for(int y = 0; y < h; ++y) {
                for(int x = 0; x < w; ++x) {
                    Vec3 s;
                    switch(i) {
                        case 0:
                            s = Vec3(half_size, y + offset - half_size,
                                     -(x + offset - half_size));
                            break;
                        case 1:
                            s = Vec3(-half_size, y + offset - half_size,
                                     x + offset - half_size);
                            break;
                        case 2:
                            s = Vec3(x + offset - half_size, -half_size,
                                     y + offset - half_size);
                            break;
                        case 3:
                            s = Vec3(x + offset - half_size, half_size,
                                     -(y + offset - half_size));
                            break;
                        case 4:
                            s = Vec3(x + offset - half_size,
                                     y + offset - half_size, half_size);
                            break;
                        case 5:
                            s = Vec3(-(x + offset - half_size),
                                     y + offset - half_size, -half_size);
                            break;
                    }

                    s.normalize();
                    float r = (s.x + 1.0f) * 0.5f;
                    float g = (s.y + 1.0f) * 0.5f;
                    float b = (s.z + 1.0f) * 0.5f;

                    data[j++] = clamp(r, 0, 1) * 255;
                    data[j++] = clamp(g, 0, 1) * 255;
                    data[j++] = clamp(b, 0, 1) * 255;
                }
            }
        }, (TextureDataOffset)(((int)TEXTURE_DATA_OFFSET_CUBE_MAP_POSITIVE_X) +
                               i));
    }

    normalization_cube_map_->flush();
}

std::shared_ptr<batcher::RenderQueueVisitor> GL1XRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<GL1RenderQueueVisitor>(this, camera);
}

smlt::GL1XRenderer::GL1XRenderer(smlt::Window *window):
    GLRenderer(window) {
}

std::shared_ptr<VertexBuffer> GL1XRenderer::prepare_vertex_data(
    MeshArrangement arrangement, const VertexData* vertex_data,
    const IndexData* index_data, const VertexRangeList* ranges) {

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
        vbuffer_data->vertices[i].t = Vec3();
        vbuffer_data->vertices[i].b = Vec3();
    }

    // Calculate vertex tangents
    auto tri_iterator = TriangleIterable(arrangement, index_data, ranges);

    Vec3 edge1, edge2;
    Vec2 deltaUV1, deltaUV2;
    Vec3 tangent;

    for(const auto& tri: tri_iterator) {
        auto& v0 = vbuffer_data->vertices[tri.idx[0]];
        auto& v1 = vbuffer_data->vertices[tri.idx[1]];
        auto& v2 = vbuffer_data->vertices[tri.idx[2]];

        edge1 = v1.xyz - v0.xyz;
        edge2 = v2.xyz - v0.xyz;

        deltaUV1 = v1.uv - v0.uv;
        deltaUV2 = v2.uv - v0.uv;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        v0.t += tangent;
        v1.t += tangent;
        v2.t += tangent;
    }

    for(uint32_t i = 0; i < vertex_count; ++i) {
        auto v = vbuffer_data->vertices[i];
        v.t.normalize();

        v.b = v.n.cross(v.t);
        v.b.normalize();
    }

    return vertex_buffer;
}
}


