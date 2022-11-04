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

#include <memory>
#include "../../generic/property.h"
#include "../../types.h"
#include "../../interfaces.h"
#include "../../interfaces/boundable.h"
#include "render_queue.h"
#include "../../generic/uniquely_identifiable.h"

namespace smlt {

class VertexData;
class IndexData;
struct VertexRange;

typedef sig::signal<void (RenderPriority, RenderPriority)> RenderPriorityChangedSignal;


class HasMutableRenderPriority {
    DEFINE_SIGNAL(RenderPriorityChangedSignal, signal_render_priority_changed);

public:
    virtual ~HasMutableRenderPriority() {}

    void set_render_priority(RenderPriority priority) {
        if(priority != render_priority_) {
            auto old = render_priority_;
            render_priority_ = priority;

            on_render_priority_changed(old, render_priority_);
            signal_render_priority_changed_(old, render_priority_);
        }
    }

    virtual RenderPriority render_priority() const { return render_priority_; }

private:
    RenderPriority render_priority_ = RENDER_PRIORITY_MAIN;

    virtual void on_render_priority_changed(
        RenderPriority old_priority, RenderPriority new_priority
    ) {
        _S_UNUSED(old_priority);
        _S_UNUSED(new_priority);
    }
};

struct alignas(8) Renderable final {
    MeshArrangement arrangement = MESH_ARRANGEMENT_TRIANGLES;
    const VertexData* vertex_data = nullptr;

    const IndexData* index_data = nullptr;
    std::size_t index_element_count = 0;

    const VertexRange* vertex_ranges = nullptr;
    std::size_t vertex_range_count = 0;

    RenderPriority render_priority = RENDER_PRIORITY_MAIN;
    Mat4 final_transformation;
    Material* material = nullptr;
    bool is_visible = true;

    LightPtr lights_affecting_this_frame[MAX_LIGHTS_PER_RENDERABLE];
    uint8_t light_count = 0;

    smlt::Vec3 centre;
};

typedef std::shared_ptr<Renderable> RenderablePtr;

}
