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
        distance_to_camera
    );
}

void GL1XRenderer::init_context() {
#ifndef _arch_dreamcast
    if(!gladLoadGL()) {
        throw std::runtime_error("Unable to intialize OpenGL 1.1");
    }
#endif

    const GLubyte* GL_vendor = glGetString(GL_VENDOR);
    const GLubyte* GL_renderer = glGetString(GL_RENDERER);
    const GLubyte* GL_version = glGetString(GL_VERSION);
    const GLubyte* GL_extensions = glGetString(GL_EXTENSIONS);

    std::cout << "\n\nOpenGL Information:\n\n";
    std::cout << _F("\tVendor: {0}\n").format(GL_vendor);
    std::cout << _F("\tRenderer: {0}\n").format(GL_renderer);
    std::cout << _F("\tVersion: {0}\n\n").format(GL_version);
    std::cout << _F("\tExtensions: {0}\n\n").format(GL_extensions);

    GLCheck(glEnable, GL_DEPTH_TEST);
    GLCheck(glDepthFunc, GL_LEQUAL);
    GLCheck(glEnable, GL_CULL_FACE);
}

std::shared_ptr<batcher::RenderQueueVisitor> GL1XRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<GL1RenderQueueVisitor>(this, camera);
}

smlt::GL1XRenderer::GL1XRenderer(smlt::Core *core):
    Renderer(core),
    GLRenderer(core) {
}

}


