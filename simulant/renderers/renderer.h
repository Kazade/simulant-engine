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

#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <vector>
#include <memory>

#include "../types.h"
#include "../generic/auto_weakptr.h"
#include "../window_base.h"

#include "batching/renderable.h"
#include "batching/render_queue.h"

namespace smlt {

class SubActor;
class HardwareBufferManager;

struct ElementRenderSpecification {
    MaterialID material_id;
    uint32_t count;
    uint8_t* indices; // Must be an array of UNSIGNED_SHORT
};

typedef std::vector<ElementRenderSpecification> ElementRenderList;

class Renderer:
    public batcher::RenderGroupFactory {

public:
    typedef std::shared_ptr<Renderer> ptr;

    Renderer(WindowBase* window):
        window_(window) {}

    virtual void render(
        CameraPtr camera,
        bool render_group_changed,
        const batcher::RenderGroup* render_group,
        Renderable* renderable,
        MaterialPass* material_pass,
        Light* light,
        const smlt::Colour& global_ambient,
        batcher::Iteration iteration
    ) = 0;

    Property<Renderer, WindowBase> window = { this, &Renderer::window_ };

    virtual void init_context() = 0;
    // virtual void upload_texture(Texture* texture) = 0;

    Property<Renderer, HardwareBufferManager> hardware_buffers = { this, [](Renderer* self) {
        return self->_get_buffer_manager();
    }};

private:    
    WindowBase* window_ = nullptr;

    virtual HardwareBufferManager* _get_buffer_manager() const = 0;
};

}

#endif // RENDERER_H
