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

namespace smlt {

class MaterialPass;
class Renderer;
class Renderable;
class Light;

namespace batcher {

/**
 * @brief The RenderGroupImpl class
 *
 * Instantiated by the renderer when renderer->new_render_group(renderable) is called.
 * Must be the renderers job to define the impl as batching varies (GL2 batches shaders for example)
 */
class RenderGroupImpl {
public:
    typedef std::shared_ptr<RenderGroupImpl> ptr;

    RenderGroupImpl(RenderPriority priority, bool is_blended, float distance_to_camera):
        priority_(priority),
        is_blended_(is_blended),
        distance_to_camera_(distance_to_camera) {

    }

    RenderGroupImpl(const RenderGroupImpl&) = delete;
    RenderGroupImpl& operator=(const RenderGroupImpl&) = delete;

    virtual ~RenderGroupImpl() {}

    bool operator<(const RenderGroupImpl& rhs) const {
        // Always sort on priority first

        if(this->priority_ < rhs.priority_) {
            return true;
        } else if(this->priority_ > rhs.priority_) {
            return false;
        }

        if(this->is_blended_ < rhs.is_blended_) {
            return true;
        } else if(this->is_blended_ > rhs.is_blended_) {
            return false;
        }

        /* Objects are sorted from farthest first, to nearest which is why this
           is reversed */
        if(this->distance_to_camera_ > rhs.distance_to_camera_) {
            return true;
        } else if(this->distance_to_camera_ < rhs.distance_to_camera_) {
            return false;
        }

        return lt(rhs);
    }

private:
    virtual bool lt(const RenderGroupImpl& rhs) const = 0;

    RenderPriority priority_;
    bool is_blended_;
    float distance_to_camera_ = 0.0f;
};

class RenderGroup {
public:
    RenderGroup() = delete;

    RenderGroup(RenderGroupImpl::ptr impl):
        impl_(impl) {

        assert(impl_);
    }

    bool operator<(const RenderGroup& rhs) const {
        // These things should never happen, but they do...
        assert(impl_);
        assert(rhs.impl_);

        return *impl_ < *rhs.impl_;
    }

    bool operator!=(const RenderGroup& rhs) const {
        // Equivilance test is fine
        if(*this < rhs) {
            return true;
        } else if(rhs < *this) {
            return true;
        } else {
            return false;
        }
    }

    RenderGroupImpl* impl() const { return impl_.get(); }

private:
    RenderGroupImpl::ptr impl_;

    friend class Renderer;
};


class RenderGroupFactory {
public:
    virtual ~RenderGroupFactory() {}

    virtual RenderGroup new_render_group(
        Renderable* renderable,
        MaterialPass* material_pass,
        RenderPriority priority,
        bool is_blended,
        float distance_to_camera
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

    virtual void visit(Renderable*, MaterialPass*, Iteration) = 0;
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

    RenderQueue(Stage* stage, RenderGroupFactory* render_group_factory, CameraPtr camera);

    void insert_renderable(std::shared_ptr<Renderable> renderable); // IMPORTANT, must update RenderGroups if they exist already
    void clear();

    void traverse(RenderQueueVisitor* callback, uint64_t frame_id) const;

    uint32_t pass_count() const { return pass_queues_.size(); }
    uint32_t group_count(Pass pass_number) const {
        uint32_t i = 0;
        auto& pass_queue = pass_queues_.at(pass_number);
        for(auto it = pass_queue.begin(); it != pass_queue.end(); it = pass_queue.upper_bound(it->first)) {
            ++i;
        }

        return i;
    }

    /*
    void each_group(Pass pass, std::function<void (uint32_t, const RenderGroup&, const Batch&)> cb) {
        uint32_t i = 0;
        for(auto& batch: batches_[pass]){
            cb(i++, batch.first, *batch.second);
        }
    } */

private:
    // std::map is ordered, so by using the RenderGroup as the key we
    // minimize GL state changes (e.g. if a RenderGroupImpl orders by TextureID, then ShaderID
    // then we'll see  (TexID(1), ShaderID(1)), (TexID(1), ShaderID(2)) for example meaning the
    // texture doesn't change even if the shader does
    typedef std::multimap<RenderGroup, std::shared_ptr<Renderable>> SortedRenderables;

    Stage* stage_ = nullptr;
    RenderGroupFactory* render_group_factory_ = nullptr;
    CameraPtr camera_;

    std::array<SortedRenderables, MAX_MATERIAL_PASSES> pass_queues_;

    void clean_empty_batches();

    mutable std::mutex queue_lock_;
};

}
}
