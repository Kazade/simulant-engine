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

#include "gskit_renderer.h"

#include "../../assets/material.h"
#include "../../material_constants.h"
#include "../../texture.h"

#include "gskit_render_queue_visitor.h"
#include "gskit_render_group_impl.h"

namespace smlt {

batcher::RenderGroupKey GSKitRenderer::prepare_render_group(
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

void GSKitRenderer::init_context() {

}

std::shared_ptr<batcher::RenderQueueVisitor> GSKitRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<GSKitRenderQueueVisitor>(this, camera);
}

smlt::GSKitRenderer::GSKitRenderer(smlt::Window *window) {
}

}


