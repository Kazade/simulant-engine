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

#include "pvr_renderer.h"

#include "../../assets/material.h"
#include "../../material_constants.h"
#include "../../texture.h"
#include "../../viewport.h"
#include "../../window.h"
#include "../utils/tex_conversions.h"
#include "pvr_api.h"
#include "pvr_render_group_impl.h"
#include "pvr_render_queue_visitor.h"

namespace smlt {

batcher::RenderGroupKey PVRRenderer::prepare_render_group(
    batcher::RenderGroup* group, const Renderable* renderable,
    const MaterialPass* material_pass, const uint8_t pass_number,
    const bool is_blended, const float distance_to_camera) {

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

#define ALIGN2048(x) ((x + (2047)) & ~2047)
#define UNCACHED(x) (x + 0x40000000)

void PVRRenderer::init_context() {
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
    pvr_start(list_);
    pvr_viewport(2048, 2048, window->width(), window->height());
    pvr_depth_func(PVR_DEPTH_FUNC_GEQUAL);
    pvr_enable(PVR_STATE_DEPTH_TEST);
    pvr_shade_model(PVR_SHADE_MODEL_SMOOTH);
    pvr_front_face(PVR_WINDING_CCW);
    pvr_finish();

    S_VERBOSE("Context initialized");
}

void PVRRenderer::on_texture_unregister(TextureID tex_id, Texture* texture) {
    texture_manager_.release_texture(texture->_renderer_specific_id());
}

void PVRRenderer::clear(const RenderTarget& target, const Colour& colour,
                        uint32_t clear_flags) {
    S_VERBOSE("clear");
    uint32_t c = int(255.0f * colour.r) | int(255.0f * colour.g) << 8 |
                 int(255.0f * colour.b) << 16 | int(255.0f * colour.a) << 24;
    uint32_t flags = 0;

    if(clear_flags & BUFFER_CLEAR_COLOUR_BUFFER) {
        flags |= PVR_CLEAR_FLAG_COLOR_BUFFER;
    }

    if(clear_flags & BUFFER_CLEAR_DEPTH_BUFFER) {
        flags |= PVR_CLEAR_FLAG_DEPTH_BUFFER;
    }

    pvr_clear_color(c);
    pvr_clear(flags);
}

void PVRRenderer::apply_viewport(const RenderTarget& target,
                                 const Viewport& viewport) {
    S_VERBOSE("apply_viewport");
    pvr_enable(PVR_STATE_SCISSOR_TEST);
    // sceGuScissor(viewport.x(), viewport.y(), viewport.width(),
    //              viewport.height());
    pvr_scissor(0, 0, 480, 272);
}

bool PVRRenderer::texture_format_is_native(TextureFormat fmt) {
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

std::shared_ptr<batcher::RenderQueueVisitor>
    PVRRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<PVRRenderQueueVisitor>(this, camera);
}

void PVRRenderer::on_pre_render() {
    S_VERBOSE("pre_render");
    texture_manager_.update_priorities();
    pvr_start(list_);
}

void PVRRenderer::on_post_render() {
    S_VERBOSE("post_render");
    pvr_finish();
}

void PVRRenderer::do_swap_buffers() {
    S_VERBOSE("do_swap_buffers");

    // sceGuSync(0, 0);
    // sceDisplayWaitVblankStart();
    // sceGuSwapBuffers();
}

void PVRRenderer::on_texture_prepare(Texture* texture) {
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
    PVRPaletteFormat palette_format = PVR_PALETTE_FORMAT_NONE;

    std::vector<uint8_t> new_data;
    std::vector<uint8_t> new_palette;

    auto tex_fmt = texture->format();

    PVRTexFormat format = PVR_TEX_FORMAT_NONE;

    if(tex_fmt == TEXTURE_FORMAT_RGB_3UB_888) {
        tex_convert_rgb888_to_bgr565(new_data, data, texture->width(),
                                     texture->height());

        data = &new_data[0];
        data_size = new_data.size();
        format = PVR_TEX_FORMAT_RGB565;
    } else if(tex_fmt == TEXTURE_FORMAT_RGB565_PALETTED4 ||
              tex_fmt == TEXTURE_FORMAT_RGB565_PALETTED8 ||
              tex_fmt == TEXTURE_FORMAT_RGB8_PALETTED4 ||
              tex_fmt == TEXTURE_FORMAT_RGB8_PALETTED8 ||
              tex_fmt == TEXTURE_FORMAT_RGBA8_PALETTED4 ||
              tex_fmt == TEXTURE_FORMAT_RGBA8_PALETTED8) {

        switch(tex_fmt) {
            case TEXTURE_FORMAT_RGB565_PALETTED8:
            case TEXTURE_FORMAT_RGB565_PALETTED4:
            case TEXTURE_FORMAT_RGBA8_PALETTED4:
            case TEXTURE_FORMAT_RGBA8_PALETTED8: {
                new_palette.resize(data_size);
                std::memcpy(&new_palette[0], data, texture->palette_size());
            } break;
            case TEXTURE_FORMAT_RGB8_PALETTED4: {
                auto w = texture->palette_size() / 3;
                tex_convert_rgb888_to_bgr565(new_palette, data, w, 1);
                format = PVR_TEX_FORMAT_PAL_4BPP;
                palette_format = PVR_PALETTE_FORMAT_RGB565;
            } break;
            case TEXTURE_FORMAT_RGB8_PALETTED8: {
                auto w = texture->palette_size() / 3;
                tex_convert_rgb888_to_bgr565(new_palette, data, w, 1);
                format = PVR_TEX_FORMAT_PAL_8BPP;
                palette_format = PVR_PALETTE_FORMAT_RGB565;
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
                format = PVR_TEX_FORMAT_RGB565;
                break;
            case TEXTURE_FORMAT_RGBA_1US_5551:
                format = PVR_TEX_FORMAT_RGBA5551;
                break;
            case TEXTURE_FORMAT_RGBA_1US_4444:
                format = PVR_TEX_FORMAT_RGBA4444;
                break;
            case TEXTURE_FORMAT_RGB565_PALETTED4:
                format = PVR_TEX_FORMAT_PAL_4BPP;
                palette_format = PVR_PALETTE_FORMAT_RGB565;
                break;
            case TEXTURE_FORMAT_RGBA8_PALETTED4:
                format = PVR_TEX_FORMAT_PAL_4BPP;
                palette_format = PVR_PALETTE_FORMAT_RGBA8888;
                break;
            case TEXTURE_FORMAT_RGB565_PALETTED8:
                format = PVR_TEX_FORMAT_PAL_8BPP;
                palette_format = PVR_PALETTE_FORMAT_RGB565;
                break;
            case TEXTURE_FORMAT_RGBA8_PALETTED8:
                format = PVR_TEX_FORMAT_PAL_8BPP;
                palette_format = PVR_PALETTE_FORMAT_RGBA8888;
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

PVRRenderer::PVRRenderer(smlt::Window* window) :
    Renderer(window), texture_manager_(this) {

    S_VERBOSE("Constructing renderer");

    sceGuInit();

    S_VERBOSE("Renderer constructed");
}
}


