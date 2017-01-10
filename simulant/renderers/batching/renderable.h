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

#include <memory>
#include "../../generic/property.h"
#include "../../types.h"
#include "../../interfaces.h"
#include "../../interfaces/boundable.h"
#include "render_queue.h"

namespace smlt {

class VertexData;
class IndexData;
class HardwareBuffer;

typedef sig::signal<void (RenderPriority, RenderPriority)> RenderPriorityChangedSignal;


class HasMutableRenderPriority {
    DEFINE_SIGNAL(RenderPriorityChangedSignal, signal_render_priority_changed);

public:
    virtual ~HasMutableRenderPriority() {}

    void set_render_priority(RenderPriority priority) {
        if(priority != render_priority_) {
            auto old = render_priority_;
            render_priority_ = priority;

            signal_render_priority_changed_(old, render_priority_);
        }
    }

    virtual RenderPriority render_priority() const { return render_priority_; }

private:
    RenderPriority render_priority_ = RENDER_PRIORITY_MAIN;
};


class Renderable:
    public batcher::BatchMember,
    public virtual BoundableEntity {

public:
    virtual ~Renderable() {}

    virtual const MeshArrangement arrangement() const = 0;

    virtual void prepare_buffers() = 0;

    virtual VertexSpecification vertex_attribute_specification() const = 0;
    virtual HardwareBuffer* vertex_attribute_buffer() const = 0;
    virtual HardwareBuffer* index_buffer() const = 0;
    virtual std::size_t index_element_count() const = 0; ///< The number of indexes that should be rendered

    virtual RenderPriority render_priority() const = 0;
    virtual Mat4 final_transformation() const = 0;

    virtual const MaterialID material_id() const = 0;
    virtual const bool is_visible() const = 0;

    void update_last_visible_frame_id(uint64_t frame_id) {
        last_visible_frame_id_ = frame_id;
    }

    bool is_visible_in_frame(uint64_t frame_id) const {
        return frame_id == last_visible_frame_id_;
    }

    void set_affected_by_lights(std::vector<LightPtr> lights) {
        lights_affecting_this_frame_ = lights;
    }

    std::vector<LightPtr> lights_affecting_this_frame() const {
        return lights_affecting_this_frame_;
    }

private:
    uint64_t last_visible_frame_id_ = 0;
    std::vector<LightPtr> lights_affecting_this_frame_;
};

typedef std::shared_ptr<Renderable> RenderablePtr;

}
