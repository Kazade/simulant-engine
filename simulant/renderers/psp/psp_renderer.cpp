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
#include <pspgum.h>
#include <pspkernel.h>

#include "psp_renderer.h"

#include "../../assets/material.h"
#include "../../material_constants.h"
#include "../../texture.h"
#include "../../window.h"
#include "../../viewport.h"
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
    S_VERBOSE("init_context");
    const int buffer_width = 512;
    const int buffer_px_bytes = 4;
    const int frame_size = buffer_width * window->height() * buffer_px_bytes;

    S_VERBOSE("W: {0} H: {1}", window->width(), window->height());

    uint8_t* start = 0;
    uint8_t* disp_buffer = start;
    uint8_t* draw_buffer = disp_buffer + frame_size;
    uint8_t* depth_buffer = draw_buffer + frame_size;

    void* texture_ram = depth_buffer + buffer_width;

    // Allocate 1.5M of VRAM for textures FIXME: Can we do more?
    vram_alloc_init(texture_ram, 1024 * 1536);

    // Set up buffers
    sceGuStart(GU_DIRECT, list_);
    sceGuDispBuffer(window->width(), window->height(), (void*) disp_buffer, buffer_width);
    sceGuDrawBuffer(GU_PSM_8888, (void*) draw_buffer, buffer_width);
    sceGuDepthBuffer(depth_buffer, buffer_width);

    sceGuOffset(2048 - (window->width() / 2), 2048 - (window->height() / 2));
    sceGuViewport(2048, 2048, window->width(), window->height());

    sceGuDepthRange(32, 65535);
    sceGuDepthFunc(GU_LEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuEnable(GU_CLIP_PLANES);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFrontFace(GU_CCW);

    sceGuClearColor(0xdddddd);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    S_VERBOSE("Context initialized");
}

void PSPRenderer::clear(const RenderTarget& target, const Colour& colour, uint32_t clear_flags) {
    S_VERBOSE("clear");
    uint32_t c = int(255.0f * colour.r) << 24 | int(255.0f * colour.g) << 16 | int(255.0f * colour.b) << 8 | int(255.0f * colour.a);
    uint32_t flags = 0;

    if(clear_flags & BUFFER_CLEAR_COLOUR_BUFFER) {
        flags |= GU_COLOR_BUFFER_BIT;
    }

    if(clear_flags & BUFFER_CLEAR_DEPTH_BUFFER) {
        flags |= GU_DEPTH_BUFFER_BIT;
    }

    sceGuClearColor(c);
    sceGuClear(flags);
}

void PSPRenderer::apply_viewport(const RenderTarget& target, const Viewport& viewport) {
    S_VERBOSE("apply_viewport");
    sceGuEnable(GU_SCISSOR_TEST);
    // sceGuScissor(viewport.x(), viewport.y(), viewport.width(),
    //              viewport.height());
    sceGuScissor(0, 0, 480, 272);
}

bool PSPRenderer::texture_format_is_native(TextureFormat fmt) {
    switch(fmt) {
        case TEXTURE_FORMAT_RGB_1US_565:
        case TEXTURE_FORMAT_RGB_3UB_888: // Converted to 565 in prepare
        case TEXTURE_FORMAT_RGBA_4UB_8888:
        case TEXTURE_FORMAT_RGBA_1US_5551:
        case TEXTURE_FORMAT_RGB565_PALETTED4:
        case TEXTURE_FORMAT_RGB565_PALETTED8:
        case TEXTURE_FORMAT_RGBA8_PALETTED4:
        case TEXTURE_FORMAT_RGBA8_PALETTED8:
            return true;
        default:
            S_ERROR("Couldn't handle format: {0}", fmt);
            return false;
    }
}

std::shared_ptr<batcher::RenderQueueVisitor> PSPRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<PSPRenderQueueVisitor>(this, camera);
}

void PSPRenderer::on_pre_render() {
    S_VERBOSE("pre_render");
    sceGuStart(GU_DIRECT, list_);
}

void PSPRenderer::on_post_render() {
    S_VERBOSE("post_render");
    sceGuFinish();
}

void PSPRenderer::do_swap_buffers() {
    S_VERBOSE("do_swap_buffers");

    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

void PSPRenderer::on_texture_prepare(Texture* texture) {
    // Do nothing if everything is up to date
    if(!texture->_data_dirty() && !texture->_params_dirty()) {
        return;
    }

    auto data = texture->data();
    const uint8_t* palette = nullptr;
    int palette_format = 0;

    std::vector<uint16_t> new_data;

    auto tex_fmt = texture->format();
    std::size_t texel_count = texture->width() * texture->height();

    if(tex_fmt == TEXTURE_FORMAT_RGB_3UB_888) {
        /* PSP doesn't natively support this so we convert to 565 when we
         * prepare */
        new_data.reserve(texel_count);

        for(std::size_t i = 0; i < texel_count; ++i) {
            auto off = (i * 3);
            uint16_t texel = ((data[off + 0] >> 3) << 11) |
                             ((data[off + 1] >> 2) << 5) |
                             ((data[off + 2] >> 3) << 0);

            new_data.push_back(texel);
        }

        data = (const uint8_t*)&new_data[0];
        tex_fmt = TEXTURE_FORMAT_RGB_1US_565;
    } else if(tex_fmt == TEXTURE_FORMAT_RGB565_PALETTED4 ||
              tex_fmt == TEXTURE_FORMAT_RGB565_PALETTED8 ||
              tex_fmt == TEXTURE_FORMAT_RGBA8_PALETTED4 ||
              tex_fmt == TEXTURE_FORMAT_RGBA8_PALETTED8) {

        const uint8_t* palette = data;
        data = palette + texture->palette_size();
    }

    S_VERBOSE("Texture format: {0}. W: {1}. H: {2}", tex_fmt, texture->width(),
              texture->height());

    int size = 0;
    int format = 0;
    switch(tex_fmt) {
        case TEXTURE_FORMAT_RGB_1US_565:
            format = GU_PSM_5650;
            size = texel_count * 2;
            break;
        case TEXTURE_FORMAT_RGBA_1US_5551:
            format = GU_PSM_5551;
            size = texel_count * 2;
            break;
        case TEXTURE_FORMAT_RGBA_4UB_8888:
            format = GU_PSM_8888;
            size = texel_count * 4;
            break;
        case TEXTURE_FORMAT_RGB565_PALETTED4:
            format = GU_PSM_T4;
            size = texel_count / 2;
            palette_format = GU_PSM_5650;
            break;
        case TEXTURE_FORMAT_RGBA8_PALETTED4:
            format = GU_PSM_T4;
            size = texel_count / 2;
            palette_format = GU_PSM_8888;
            break;
        case TEXTURE_FORMAT_RGB565_PALETTED8:
            format = GU_PSM_T8;
            size = texel_count;
            palette_format = GU_PSM_5650;
            break;
        case TEXTURE_FORMAT_RGBA8_PALETTED8:
            format = GU_PSM_T8;
            size = texel_count;
            palette_format = GU_PSM_8888;
            break;
        default:
            S_ERROR("Unsupported texture format for PSP: {0}",
                    texture->format());
            return;
    }

    auto id = texture_manager_.upload_texture(
        texture->_renderer_specific_id(), // If 0, the texture will be created
        format, texture->width(), texture->height(), size, data, palette,
        palette_format);

    texture->_set_renderer_specific_id(id);
    texture->_set_data_clean();
    texture->_set_params_clean();
}

PSPRenderer::PSPRenderer(smlt::Window* window) :
    Renderer(window), texture_manager_(this) {

    S_VERBOSE("Constructing renderer");

    sceGuInit();

    S_VERBOSE("Renderer constructed");
}
}


