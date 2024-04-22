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

#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>

#include "psp_renderer.h"

#include "../../assets/material.h"
#include "../../material_constants.h"
#include "../../texture.h"
#include "../../window.h"
#include "psp_render_group_impl.h"
#include "psp_render_queue_visitor.h"

namespace smlt {

batcher::RenderGroupKey PSPRenderer::prepare_render_group(
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

void PSPRenderer::init_context() {

}

bool PSPRenderer::texture_format_is_native(TextureFormat fmt) {
    fprintf(stderr, "Stub texture_format_is_native");
    return true;
}


std::shared_ptr<batcher::RenderQueueVisitor> PSPRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<PSPRenderQueueVisitor>(this, camera);
}

PSPRenderer::PSPRenderer(smlt::Window *window):
    Renderer(window){

    const int buffer_width = 512;

    sceGuInit();

    // Set up buffers
    sceGuStart(GU_DIRECT, list_);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, buffer_width);
    sceGuDispBuffer(window->width(), window->height(), (void*)0x88000,
                    buffer_width);
    sceGuDepthBuffer((void*)0x110000, buffer_width);

    sceGuOffset(2048 - (window->width() / 2), 2048 - (window->height() / 2));
    sceGuViewport(2048, 2048, window->width(), window->height());
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuScissor(0, 0, window->width(), window->height());

    sceGuDepthRange(65535, 0);

    sceGuDepthFunc(GU_GEQUAL);  // Reversed, so GEQUAL instead of LEQUAL
    sceGuEnable(GU_DEPTH_TEST); // Enable depth testing

    sceGuFinish();
    sceGuDisplay(GU_TRUE);
}
}


