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

#include "../../types.h"
#include "../../threads/shared_mutex.h"
#include "../../macros.h"

namespace smlt {

class MaterialPass;
class Renderer;
struct Renderable;
class Light;
class StageNode;

namespace batcher {


/* How render ordering works:
 *
 * 1. Pass number takes precendence
 * 2. Within a pass, blended objects are rendered first
 * 3. If an object is not blended, objects order by distance to camera least, to most. If blended, most to least
 * 4. If distance to camera is equal, z order is taken into account
*/
struct RenderGroupKey {
    uint8_t pass = 0; // 1 byte
    bool is_blended = false; // 1 byte
    float distance_to_camera = 0.0f; // 4 bytes
    int16_t precedence = 0; // 2-bytes to get 8-byte alignment
};


struct RenderGroup {
    /* A sort key, generated from priority and material properties, this
     * may differ per-renderer */
    RenderGroupKey sort_key;

    bool operator<(const RenderGroup& rhs) const {
        if(sort_key.pass < rhs.sort_key.pass) {
            return true;
        } else if(sort_key.pass > rhs.sort_key.pass) {
            return false;
        }

        if(sort_key.is_blended < rhs.sort_key.is_blended) {
            return true;
        } else if(sort_key.is_blended > rhs.sort_key.is_blended) {
            return false;
        }

        if(!sort_key.is_blended) {
            if(sort_key.distance_to_camera < rhs.sort_key.distance_to_camera) {
                // If the object is opaque, we want to render
                // front-to-back, so less distance is less
                return true;
            } else if(sort_key.distance_to_camera > rhs.sort_key.distance_to_camera) {
                return false;
            }

            return sort_key.precedence > rhs.sort_key.precedence;
        } else {
            if(rhs.sort_key.distance_to_camera < sort_key.distance_to_camera) {
                // If the object is translucent, we want to render
                // back-to-front
                return true;
            } else if(rhs.sort_key.distance_to_camera > sort_key.distance_to_camera) {
                return false;
            }

            return sort_key.precedence > rhs.sort_key.precedence;
        }

        return false;
    }

    bool operator==(const RenderGroup& rhs) const  {
        return (
            sort_key.pass == rhs.sort_key.pass &&
            sort_key.is_blended == rhs.sort_key.is_blended &&
            almost_equal(sort_key.distance_to_camera, rhs.sort_key.distance_to_camera) &&
            sort_key.precedence == rhs.sort_key.precedence
        );
    }

    bool operator!=(const RenderGroup& rhs) const {
        return !(*this == rhs);
    }
};

RenderGroupKey generate_render_group_key(const uint8_t pass, const bool is_blended, const float distance_to_camera, int16_t precedence);

class RenderGroupFactory {
public:
    virtual ~RenderGroupFactory() {}

    /* Initialize a render group based on the provided arguments
     * Returns the group's sort_key */
    virtual RenderGroupKey prepare_render_group(
        RenderGroup* group,
        const Renderable* renderable,
        const MaterialPass* material_pass,
        const uint8_t pass_number,
        const bool is_blended,
        const float distance_to_camera
    ) = 0;
};

typedef uint32_t Pass;
typedef uint32_t Iteration;

class RenderQueue;

class RenderQueueVisitor {
protected:
    Renderer* renderer() const {
        return renderer_;
    }

public:
    RenderQueueVisitor(Renderer* renderer) :
        renderer_(renderer) {}

    virtual ~RenderQueueVisitor() {}

    virtual void start_traversal(const RenderQueue& queue, uint64_t frame_id, StageNode* stage) = 0;

    virtual void change_render_group(const RenderGroup* prev, const RenderGroup* next) = 0;

    virtual void change_material_pass(const MaterialPass* prev, const MaterialPass* next) = 0;
    virtual void apply_lights(const LightPtr* lights, const uint8_t count) = 0;

    void visit(const Renderable* renderable, const MaterialPass* pass,
               Iteration iteration);
    virtual void end_traversal(const RenderQueue& queue, StageNode* stage) = 0;

private:
    Renderer* renderer_ = nullptr;

    virtual void do_visit(const Renderable*, const MaterialPass*,
                          Iteration) = 0;
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

    std::size_t queue_count() const { return priority_queues_.size(); }
    std::size_t group_count(Pass pass_number) const;

    std::size_t renderable_count() const { return renderables_.size(); }
    Renderable* renderable(const std::size_t i) {
        return &renderables_[i];
    }
private:
    // std::map is ordered, so by using the RenderGroup as the key we
    // minimize GL state changes (e.g. if a RenderGroupImpl orders by AssetID, then ShaderID
    // then we'll see  (TexID(1), ShaderID(1)), (TexID(1), ShaderID(2)) for example meaning the
    // texture doesn't change even if the shader does
    typedef ContiguousMultiMap<RenderGroup, std::size_t> SortedRenderables;

    StageNode* stage_node_ = nullptr;
    RenderGroupFactory* render_group_factory_ = nullptr;
    CameraPtr camera_;

    std::vector<Renderable> renderables_;
    std::array<SortedRenderables, RENDER_PRIORITY_MAX - RENDER_PRIORITY_MIN> priority_queues_;

    void clean_empty_batches();

    mutable thread::Mutex queue_lock_;
};

}
}
