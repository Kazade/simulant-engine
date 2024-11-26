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

#include <list>
#include <set>

#include "../../generic/containers/contiguous_map.h"

#include "../../core/aligned_allocator.h"
#include "../../macros.h"
#include "../../threads/shared_mutex.h"
#include "../../types.h"

namespace smlt {

class MaterialPass;
class Renderer;
struct Renderable;
class Light;
class StageNode;

namespace batcher {

/* Remember! Render group keys are for *sorting* only. Priority, pass, and
is_blended are the only values you
 * can rely on to be accurate, the others are capped or limited in some way. We
order in the following way:
 *
 *  - RenderPriority. This is the highest level ordering, renderables with
different priorities are basically in different "queues"
 *  - Pass. This is the material pass number, the lower the pass number, the
earlier it is rendered
 *  - is_blended. This is 0 if the object is opaque or 1 if it's translucent.
This impacts the next value.
 *  - distance to camera. Stored as an unsigned 10-bit minifloat, with 5
exponent bits and 5 mantissa bits. If the object is blended, this is the
max-storable float minus the distance to camera)
 *  - Precedence. This is a value that allows you to tweak the order of objects
*/

struct RenderGroupKey {
    union {
        struct {
            // Min/max priority is -/+ 25, but we store as 0 - 50.
            unsigned priority : 6;
            unsigned pass : 2; // Max material passes is 4
            unsigned is_blended : 1;

            // Float10 value for the distance to camera. If is_blended is true
            // then this will be Float10::max_value - value
            unsigned distance_to_camera : 10;

            // This allows 7 levels of tweaking the
            unsigned precedence : 3;

            // Texture ID of the base color texture. This is used
            // to minimize texture changes, if you have a lot of
            // textures this will lose its value
            unsigned texture : 10;
        } s;
        uint32_t i;
    };
};

struct RenderGroup {
    /* A sort key, generated from priority and material properties, this
     * may differ per-renderer */
    RenderGroupKey sort_key;

    bool operator<(const RenderGroup& rhs) const {
        return sort_key.i < rhs.sort_key.i;
    }

    bool operator==(const RenderGroup& rhs) const  {
        return sort_key.i == rhs.sort_key.i;
    }

    bool operator!=(const RenderGroup& rhs) const {
        return !(*this == rhs);
    }
};

RenderGroupKey generate_render_group_key(const RenderPriority priority,
                                         const uint8_t pass,
                                         const bool is_blended,
                                         const float distance_to_camera,
                                         int16_t precedence);

class RenderGroupFactory {
public:
    virtual ~RenderGroupFactory() {}

    /* Initialize a render group based on the provided arguments
     * Returns the group's sort_key */
    virtual RenderGroupKey
        prepare_render_group(RenderGroup* group, const Renderable* renderable,
                             const MaterialPass* material_pass,
                             const RenderPriority priority,
                             const uint8_t pass_number, const bool is_blended,
                             const float distance_to_camera) = 0;
};

typedef uint32_t Pass;
typedef uint32_t Iteration;

class RenderQueue;

class RenderQueueVisitor {
public:
    virtual ~RenderQueueVisitor() {}

    virtual void start_traversal(const RenderQueue& queue, uint64_t frame_id, StageNode* stage) = 0;

    virtual void change_render_group(const RenderGroup* prev, const RenderGroup* next) = 0;

    virtual void change_material_pass(const MaterialPass* prev, const MaterialPass* next) = 0;
    virtual void apply_lights(const LightPtr* lights, const uint8_t count) = 0;

    virtual void visit(const Renderable*, const MaterialPass*, Iteration) = 0;
    virtual void end_traversal(const RenderQueue& queue, StageNode* stage) = 0;
};


class MaterialChangeWatcher {
public:
    MaterialChangeWatcher(RenderQueue* queue):
        queue_(queue) {}

    void watch(AssetID material_id, Renderable* renderable);
    void unwatch(Renderable* renderable);

private:
    RenderQueue* queue_;

    /*
     * We store a list of all the renderables that need to be reinserted if a material changes
     */
    std::unordered_map<AssetID, std::set<Renderable*>> renderables_by_material_;

    /*
     * We store connections to material update signals, when all renderables are removed
     * for a particular material, we disconnect the signal
     */
    std::unordered_map<AssetID, sig::connection> material_update_conections_;

    void on_material_changed(AssetID material);
};


class RenderQueue {
public:
    typedef std::function<void (bool, const RenderGroup*, Renderable*, MaterialPass*, Light*, Iteration)> TraverseCallback;

    RenderQueue();

    void reset(StageNode* stage, RenderGroupFactory* render_group_factory, CameraPtr camera);

    void insert_renderable(Renderable&& renderable); // IMPORTANT, must update RenderGroups if they exist already
    void clear();

    void traverse(RenderQueueVisitor* callback, uint64_t frame_id) const;

    std::size_t renderable_count() const {
        return render_queue_.size();
    }

    Renderable* renderable(std::size_t idx);

private:
    // std::map is ordered, so by using the RenderGroup as the key we
    // minimize GL state changes (e.g. if a RenderGroupImpl orders by AssetID, then ShaderID
    // then we'll see  (TexID(1), ShaderID(1)), (TexID(1), ShaderID(2)) for example meaning the
    // texture doesn't change even if the shader does
    typedef ContiguousMultiMap<RenderGroup, Renderable> SortedRenderables;

    StageNode* stage_node_ = nullptr;
    RenderGroupFactory* render_group_factory_ = nullptr;
    CameraPtr camera_;

    SortedRenderables render_queue_;

    void clean_empty_batches();

    mutable thread::Mutex queue_lock_;
};

}
}
