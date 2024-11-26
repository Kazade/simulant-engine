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
#include "../../viewport.h"
#include "../../window.h"
#include "../utils/tex_conversions.h"
#include "psp_render_group_impl.h"
#include "psp_render_queue_visitor.h"

namespace smlt {

batcher::RenderGroupKey PSPRenderer::prepare_render_group(
    batcher::RenderGroup* group, const Renderable* renderable,
    const MaterialPass* material_pass, const RenderPriority priority,
    const uint8_t pass_number, const bool is_blended,
    const float distance_to_camera) {

    _S_UNUSED(renderable);
    _S_UNUSED(material_pass);
    _S_UNUSED(group);

    return batcher::generate_render_group_key(priority, pass_number, is_blended,
                                              distance_to_camera,
                                              renderable->precedence);
}

#define ALIGN2048(x) ((x + (2047)) & ~2047)
#define UNCACHED(x) (x + 0x40000000)

void PSPRenderer::init_context() {
    S_VERBOSE("init_context");
    const int buffer_width = 512;
    const int buffer_px_bytes = 4;
    const int frame_size = buffer_width * window->height() * buffer_px_bytes;
    const int depth_size = 512 * 272 * 2;

    uint8_t* start = (uint8_t*)sceGeEdramGetAddr();
    uint8_t* disp_buffer = start;
    uint8_t* draw_buffer = disp_buffer + frame_size;
    uint8_t* depth_buffer = draw_buffer + frame_size;
    uint8_t* texture_ram = depth_buffer + depth_size;
    texture_ram = (uint8_t*)ALIGN2048((intptr_t)(texture_ram));

    S_VERBOSE("Ram size: {0}", sceGeEdramGetSize());

    auto max_size = sceGeEdramGetSize() - intptr_t(texture_ram - start);

    S_VERBOSE("Using {0} bytes of VRAM for texture pool", max_size);

    vram_alloc_init(UNCACHED(texture_ram), max_size);

    // Set up buffers
    sceGuStart(GU_DIRECT, list_);
    sceGuDispBuffer(window->width(), window->height(),
                    (void*)(disp_buffer - start), buffer_width);
    sceGuDrawBuffer(GU_PSM_8888, (void*)(draw_buffer - start), buffer_width);
    sceGuDepthBuffer(depth_buffer, buffer_width);

    sceGuOffset(2048 - (window->width() / 2), 2048 - (window->height() / 2));
    sceGuViewport(2048, 2048, window->width(), window->height());

    sceGuDepthRange(65535, 0);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuEnable(GU_CLIP_PLANES);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFrontFace(GU_CCW);

    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    S_VERBOSE("Context initialized");
}

void PSPRenderer::on_texture_unregister(AssetID tex_id, Texture *texture)
{
    texture_manager_.release_texture(texture->_renderer_specific_id());
}

void PSPRenderer::clear(const RenderTarget &target, const Color &color, uint32_t clear_flags)
{
    S_VERBOSE("clear");
    uint32_t c = int(255.0f * color.r) | int(255.0f * color.g) << 8 | int(255.0f * color.b) << 16
                 | int(255.0f * color.a) << 24;
    uint32_t flags = 0;

    if (clear_flags & BUFFER_CLEAR_COLOR_BUFFER) {
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
        case TEXTURE_FORMAT_RGBA_1US_4444:
        case TEXTURE_FORMAT_ARGB_1US_4444:
        case TEXTURE_FORMAT_RGB565_PALETTED4:
        case TEXTURE_FORMAT_RGB565_PALETTED8:
        case TEXTURE_FORMAT_RGBA8_PALETTED4:
        case TEXTURE_FORMAT_RGBA8_PALETTED8:
            return true;
        default:
            return false;
    }
}

std::shared_ptr<batcher::RenderQueueVisitor> PSPRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<PSPRenderQueueVisitor>(this, camera);
}

void PSPRenderer::on_pre_render() {
    S_VERBOSE("pre_render");
    texture_manager_.update_priorities();
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

    if(texture->width() >= 256 || texture->height() >= 256) {
        S_ERROR("Large texture: {0}", texture->source().str());
    }

    auto data = texture->data();
    auto data_size = texture->data_size();
    const uint8_t* palette = nullptr;
    int palette_format = 0;

    std::vector<uint8_t> new_data;
    std::vector<uint8_t> new_palette;

    auto tex_fmt = texture->format();
    std::size_t texel_count = texture->width() * texture->height();

    int format = -1;

    if(tex_fmt == TEXTURE_FORMAT_RGB_3UB_888) {
        tex_convert_rgb888_to_bgr565(new_data, data, texture->width(),
                                     texture->height());

        data = &new_data[0];
        data_size = new_data.size();
        format = GU_PSM_5650;
    } else if(tex_fmt == TEXTURE_FORMAT_ARGB_1US_4444) {
        tex_convert_argb4444_to_abgr4444(new_data, data, texture->width(),
                                         texture->height());

        data = &new_data[0];
        format = GU_PSM_4444;
    } else if(tex_fmt == TEXTURE_FORMAT_RGB_1US_565) {
        tex_convert_rgb565_to_bgr565(new_data, data, texture->width(),
                                     texture->height());

        data = &new_data[0];
        format = GU_PSM_5650;
    } else if(tex_fmt == TEXTURE_FORMAT_RGB565_PALETTED4 ||
              tex_fmt == TEXTURE_FORMAT_RGB565_PALETTED8 ||
              tex_fmt == TEXTURE_FORMAT_RGB8_PALETTED4 ||
              tex_fmt == TEXTURE_FORMAT_RGB8_PALETTED8 ||
              tex_fmt == TEXTURE_FORMAT_RGBA8_PALETTED4 ||
              tex_fmt == TEXTURE_FORMAT_RGBA8_PALETTED8) {

        switch(tex_fmt) {
            case TEXTURE_FORMAT_RGBA8_PALETTED4:
            case TEXTURE_FORMAT_RGBA8_PALETTED8: {
                new_palette.resize(data_size);
                std::memcpy(&new_palette[0], data, texture->palette_size());
            } break;
            case TEXTURE_FORMAT_RGB8_PALETTED4: {
                auto w = texture->palette_size() / 3;
                tex_convert_rgb888_to_bgr565(new_palette, data, w, 1);
                format = GU_PSM_T4;
                palette_format = GU_PSM_5650;
            } break;
            case TEXTURE_FORMAT_RGB8_PALETTED8: {
                auto w = texture->palette_size() / 3;
                tex_convert_rgb888_to_bgr565(new_palette, data, w, 1);
                format = GU_PSM_T8;
                palette_format = GU_PSM_5650;
            } break;
            case TEXTURE_FORMAT_RGB565_PALETTED8:
            case TEXTURE_FORMAT_RGB565_PALETTED4: {
                auto w = texture->palette_size() / 2;
                tex_convert_rgb565_to_bgr565(new_palette, data, w, 1);
            } break;
            default:
                break;
        }

        palette = &new_palette[0];
        data = data + texture->palette_size();
        data_size -= texture->palette_size();

        /* Very strangely, PSP 4bpp format has each nibble reversed compared
         * to DC and PC(?) */
        if(tex_fmt == TEXTURE_FORMAT_RGBA8_PALETTED4 ||
           tex_fmt == TEXTURE_FORMAT_RGB8_PALETTED4 ||
           tex_fmt == TEXTURE_FORMAT_RGB565_PALETTED4) {
            for(std::size_t i = 0; i < data_size; ++i) {
                new_data.push_back((data[i] >> 4) | (data[i] & 0xF) << 4);
            }

            data = &new_data[0];
        }
    }

    S_VERBOSE("Texture format: {0}. W: {1}. H: {2}", tex_fmt, texture->width(),
              texture->height());

    if(format < 0) {
        switch(tex_fmt) {
            case TEXTURE_FORMAT_RGB_1US_565:
                format = GU_PSM_5650;
                break;
            case TEXTURE_FORMAT_RGBA_1US_5551:
                format = GU_PSM_5551;
                break;
            case TEXTURE_FORMAT_RGBA_1US_4444:
                format = GU_PSM_4444;
                break;
            case TEXTURE_FORMAT_RGBA_4UB_8888:
                format = GU_PSM_8888;
                break;
            case TEXTURE_FORMAT_RGB565_PALETTED4:
                format = GU_PSM_T4;
                palette_format = GU_PSM_5650;
                break;
            case TEXTURE_FORMAT_RGBA8_PALETTED4:
                format = GU_PSM_T4;
                palette_format = GU_PSM_8888;
                break;
            case TEXTURE_FORMAT_RGB565_PALETTED8:
                format = GU_PSM_T8;
                palette_format = GU_PSM_5650;
                break;
            case TEXTURE_FORMAT_RGBA8_PALETTED8:
                format = GU_PSM_T8;
                palette_format = GU_PSM_8888;
                break;
            default:
                S_ERROR("Unsupported texture format for PSP: {0}",
                        texture->format());
                return;
        }
    }

    auto id = texture_manager_.upload_texture(
        texture->_renderer_specific_id(), // If 0, the texture will be created
        format, texture->width(), texture->height(), data_size, data, palette,
        texture->palette_size(), palette_format,
        texture->mipmap_generation() == MIPMAP_GENERATE_COMPLETE);

    if(id > 0) {
        texture->_set_renderer_specific_id(id);
    }

    // We always set this clean, even on failure otherwise we'll keep
    // retrying
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


