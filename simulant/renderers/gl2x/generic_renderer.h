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

#ifndef GENERIC_RENDERER_H
#define GENERIC_RENDERER_H

#include <vector>

#include "../renderer.h"
#include "../../material.h"
#include "./buffer_manager.h"

namespace smlt {

class GenericRenderer : public Renderer {
public:
    GenericRenderer(WindowBase* window):
        Renderer(window),
        buffer_manager_(new GL2BufferManager(this)) {

    }

    batcher::RenderGroup new_render_group(Renderable *renderable, MaterialPass *material_pass);

    void render(CameraPtr camera, bool render_group_changed,
        const batcher::RenderGroup *,
        Renderable*,
        MaterialPass*,
        Light*,
        const smlt::Colour& global_ambient,
        batcher::Iteration
    ) override;

    void init_context();

private:
    std::unique_ptr<HardwareBufferManager> buffer_manager_;

    HardwareBufferManager* _get_buffer_manager() const {
        return buffer_manager_.get();
    }

    void set_light_uniforms(GPUProgramInstance* program_instance, Light* light);
    void set_material_uniforms(GPUProgramInstance* program_instance, MaterialPass *pass);
    void set_auto_uniforms_on_shader(GPUProgramInstance *pass, CameraPtr camera, Renderable* subactor, const Colour &global_ambient);
    void set_auto_attributes_on_shader(Renderable &buffer);
    void set_blending_mode(BlendType type);
    void send_geometry(Renderable* renderable);
};

}

#endif // GENERIC_RENDERER_H
