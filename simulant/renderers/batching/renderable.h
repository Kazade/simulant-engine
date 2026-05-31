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

/* Renderer-agnostic per-renderable hints. Renderers that don't recognise a
 * given flag simply ignore it. */
enum RenderableFlags : uint32_t {
    /* This renderable receives shadows. On the PVR (Dreamcast) it causes the
     * polygon header to be patched with the modifier-enable bit so cheap-shadow
     * modifier volumes darken it. Other renderers currently ignore it. */
    RENDERABLE_FLAG_RECEIVES_SHADOWS = 1u << 0,
};

struct alignas(8) Renderable final {
    /* A stable identifier for persistent renderables. If != -1, this renderable
     * is expected to be returned again on subsequent frames with the same
     * geometry, so derived data (e.g. shadow adjacency) may be cached against
     * this key. If == -1 the renderable is transient and must not be cached.
     *
     * The key must be globally unique amongst persistent renderables. Nodes
     * backed by fixed geometry (e.g. Actor) should return the same key for the
     * same geometry every frame. */
    int64_t key = -1;

    /* Bitmask of RenderableFlags. Populated by the producing node. */
    uint32_t flags = 0;

    MeshArrangement arrangement = MESH_ARRANGEMENT_TRIANGLES;
    const VertexData* vertex_data = nullptr;

    const IndexData* index_data = nullptr;
    std::size_t index_element_count = 0;
    /* Offset (in index elements, not bytes) into index_data at which this
     * renderable's draw begins. Lets several renderables share one index buffer
     * by pointing at different slices — used by e.g. ShadowCaster to submit
     * each shadow caster as a separate PVR modifier volume. Default 0 means
     * "start at the beginning", which matches every existing producer. */
    std::size_t first_index = 0;

    const VertexRange* vertex_ranges = nullptr;
    std::size_t vertex_range_count = 0;

    RenderPriority render_priority = RENDER_PRIORITY_MAIN;
    const Mat4* final_transformation = nullptr;
    Material* material = nullptr;
    bool is_visible = true;

    LightPtr lights_affecting_this_frame[MAX_LIGHTS_PER_RENDERABLE];
    uint8_t light_count = 0;

    smlt::Vec3 center;
    float precedence = 0.0f;
};

typedef std::shared_ptr<Renderable> RenderablePtr;

}
