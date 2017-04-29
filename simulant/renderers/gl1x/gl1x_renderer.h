/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../renderer.h"

namespace smlt {

class GL1XRenderer : public Renderer {
public:
    GL1XRenderer(WindowBase* window):
        Renderer(window) {}

    batcher::RenderGroup new_render_group(Renderable *renderable, MaterialPass *material_pass);

    void render(CameraPtr camera, bool render_group_changed,
        const batcher::RenderGroup *,
        Renderable* renderable,
        MaterialPass* material_pass,
        Light* light,
        const Colour& global_ambient,
        batcher::Iteration iteration
    ) override;

    void init_context();

    void render_triangle_buffers(
        VertexSpecification vertex_format,
        uint8_t* vertex_data,
        const ElementRenderList& render_list
    );
};

}
