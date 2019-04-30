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
#include "../gl_renderer.h"

namespace smlt {


class GL1XRenderer:
    public Renderer,
    public GLRenderer {

public:
    friend class GL1RenderQueueVisitor;

    GL1XRenderer(Window* window);

    batcher::RenderGroup new_render_group(
        Renderable *renderable,
        MaterialPass *material_pass,
        RenderPriority priority,
        bool is_blended,
        float distance_to_camera
    ) override;
    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera);

    void init_context();

    std::string name() const override {
        return "gl1x";
    }

    void prepare_to_render(Renderable *renderable) override {}
private:
    void on_texture_prepare(TexturePtr texture) override {
        GLRenderer::on_texture_prepare(texture);
    }

    void on_texture_register(TextureID tex_id, TexturePtr texture) override {
        GLRenderer::on_texture_register(tex_id, texture);
    }

    void on_texture_unregister(TextureID tex_id) override {
        GLRenderer::on_texture_unregister(tex_id);
    }
};

}
