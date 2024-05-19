/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../renderer.h"
#include "../utils/vram_alloc.h"
#include "pvr_texture_manager.h"

namespace smlt {

class PVRRenderer: public Renderer {

public:
    friend class PSPRenderQueueVisitor;

    PVRRenderer(Window* window);

    batcher::RenderGroupKey prepare_render_group(batcher::RenderGroup* group,
        const Renderable *renderable,
        const MaterialPass *material_pass,
        const uint8_t pass_number,
        const bool is_blended,
        const float distance_to_camera) override;

    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera) override;

    void init_context() override;

    std::string name() const override {
        return "psp";
    }

    const uint8_t* display_list() const {
        return list_;
    }

    bool texture_format_is_native(TextureFormat fmt) override;

    void clear(const RenderTarget& target, const Colour& colour,
               uint32_t clear_flags) override;
    void apply_viewport(const RenderTarget& target,
                        const Viewport& viewport) override;

    std::size_t max_texture_size() const override {
        return 512;
    }

    void prepare_to_render(const Renderable*) override {}

private:    
    uint8_t list_[512 * 1024] __attribute__((aligned(64)));

    PVRTextureManager texture_manager_;

    void on_pre_render() override;
    void on_post_render() override;
    void do_swap_buffers() override;

    void on_texture_prepare(Texture* texture) override;
    void on_texture_unregister(TextureID tex_id, Texture* texture) override;

    void* vram_base_ = nullptr;
};
}


