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
#include <map>

#include "../../types.h"
#include "../../generic/threading/shared_mutex.h"
#include "../../generic/vector_pool.h"
#include "../../macros.h"

namespace smlt {

class MaterialPass;
class Renderer;
struct Renderable;
class Light;

namespace batcher {

struct RenderGroupKey {
    uint8_t pass;
    bool is_blended;
    float distance_to_camera;

    bool operator==(const RenderGroupKey& rhs) const  {
        return memcmp(this, &rhs, sizeof(RenderGroupKey)) == 0;
    }

    bool operator!=(const RenderGroupKey& rhs) const {
        return !(*this == rhs);
    }

    bool operator<(const RenderGroupKey& rhs) const;
};


struct RenderGroup {
    /* FIXME: VectorPool wasn't written generically and assumes we're using
     * UniqueID. This hacks around that for now. */
    struct ID {
        uint32_t id_;
        ID(uint32_t id):
            id_(id) {}

        uint32_t value() const {
            return id_;
        }
    };

    // FIXME: VectorPool expects arguments
    RenderGroup(RenderGroup::ID, int dummy) {
        _S_UNUSED(dummy);
    }

    /* A sort key, generated from priority and material properties, this
     * may differ per-renderer */
    RenderGroupKey sort_key;

    bool operator<(const RenderGroup& rhs) const {
        return sort_key < rhs.sort_key;
    }

    bool operator==(const RenderGroup& rhs) const {
        return sort_key == rhs.sort_key;
    }

    bool operator!=(const RenderGroup& rhs) const {
        return sort_key != rhs.sort_key;
    }

    // FIXME: Make VectorPool generic so this isn't necessary */
    bool init() { return true; }
    void clean_up() {}
};

RenderGroupKey generate_render_group_key(const uint8_t pass, const bool is_blended, const float distance_to_camera);

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
public:
    virtual ~RenderQueueVisitor() {}

    virtual void start_traversal(const RenderQueue& queue, uint64_t frame_id, Stage* stage) = 0;

    virtual void change_render_group(const RenderGroup* prev, const RenderGroup* next) = 0;

    virtual void change_material_pass(const MaterialPass* prev, const MaterialPass* next) = 0;
    virtual void apply_lights(const LightPtr* lights, const uint8_t count) = 0;

    virtual void visit(const Renderable*, const MaterialPass*, Iteration) = 0;
    virtual void end_traversal(const RenderQueue& queue, Stage* stage) = 0;
};


class MaterialChangeWatcher {
public:
    MaterialChangeWatcher(RenderQueue* queue):
        queue_(queue) {}

    void watch(MaterialID material_id, Renderable* renderable);
    void unwatch(Renderable* renderable);

private:
    RenderQueue* queue_;

    /*
     * We store a list of all the renderables that need to be reinserted if a material changes
     */
    std::unordered_map<MaterialID, std::set<Renderable*>> renderables_by_material_;

    /*
     * We store connections to material update signals, when all renderables are removed
     * for a particular material, we disconnect the signal
     */
    std::unordered_map<MaterialID, sig::connection> material_update_conections_;

    void on_material_changed(MaterialID material);
};


class RenderQueue {
public:
    typedef std::function<void (bool, const RenderGroup*, Renderable*, MaterialPass*, Light*, Iteration)> TraverseCallback;

    RenderQueue();

    void reset(Stage* stage, RenderGroupFactory* render_group_factory, CameraPtr camera);

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
    // minimize GL state changes (e.g. if a RenderGroupImpl orders by TextureID, then ShaderID
    // then we'll see  (TexID(1), ShaderID(1)), (TexID(1), ShaderID(2)) for example meaning the
    // texture doesn't change even if the shader does

    struct Compare {
        bool operator()(RenderGroup* lhs, RenderGroup* rhs) const {
            assert(lhs);
            assert(rhs);

            return *lhs < *rhs;
        }
    };

    typedef std::multimap<RenderGroup*, std::size_t, Compare> SortedRenderables;

    Stage* stage_ = nullptr;
    RenderGroupFactory* render_group_factory_ = nullptr;
    CameraPtr camera_;

    std::vector<Renderable> renderables_;
    std::array<SortedRenderables, RENDER_PRIORITY_MAX - RENDER_PRIORITY_MIN> priority_queues_;

    void clean_empty_batches();

    mutable std::mutex queue_lock_;

    VectorPool<RenderGroup, RenderGroup::ID, 128> render_group_pool_;
};

}
}
