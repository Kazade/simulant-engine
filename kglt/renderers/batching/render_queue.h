#pragma once

#include <list>
#include <set>
#include <map>

namespace kglt {

class MaterialPass;
class Renderer;
class Renderable;
class Light;

namespace batcher {

class Batch;

class BatchMember {
public:
    virtual ~BatchMember() {}

    void join_batch(Batch* batch) {
        batches_.insert(batch);
    }

    void leave_batch(Batch* batch) {
        batches_.erase(batch);
    }

    std::set<Batch*> batches() const { return batches_; }

private:
    std::set<Batch*> batches_;
};

class Batch {
public:
    void add_renderable(Renderable* renderable);
    void remove_renderable(Renderable* renderable);

    void each(std::function<void (uint32_t, Renderable*)> func) const {
        uint32_t i = 0;
        for(auto& renderable: renderables_) {
            func(i++, renderable);
        }
    }

    uint32_t renderable_count() const { return renderables_.size(); }

private:
    std::list<Renderable*> renderables_;
};

/**
 * @brief The RenderGroupImpl class
 *
 * Instantiated by the renderer when renderer->new_render_group(renderable) is called.
 * Must be the renderers job to define the impl as batching varies (GL2 batches shaders for example)
 */
class RenderGroupImpl {
public:
    typedef std::shared_ptr<RenderGroupImpl> ptr;

    RenderGroupImpl(RenderPriority priority):
        priority_(priority) {

    }

    RenderGroupImpl(const RenderGroupImpl&) = delete;
    RenderGroupImpl& operator=(const RenderGroupImpl&) = delete;

    virtual ~RenderGroupImpl() {}

    bool operator<(const RenderGroupImpl& rhs) const {
        // Always sort on priority first

        if(this->priority_ < rhs.priority_) {
            return true;
        }

        return lt(rhs);
    }

private:
    virtual bool lt(const RenderGroupImpl& rhs) const = 0;

    RenderPriority priority_;
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

    RenderGroupImpl* impl() const { return impl_.get(); }

private:
    RenderGroupImpl::ptr impl_;

    friend class Renderer;
};


class RenderGroupFactory {
public:
    virtual ~RenderGroupFactory() {}

    virtual RenderGroup new_render_group(Renderable* renderable, MaterialPass* material_pass) = 0;
};

typedef uint32_t Pass;
typedef uint32_t Iteration;

class RenderQueue {
public:
    typedef std::function<void (bool, const RenderGroup*, Renderable*, MaterialPass*, Light*, Iteration)> TraverseCallback;

    RenderQueue(Stage* stage, RenderGroupFactory* render_group_factory);

    void insert_renderable(Renderable* renderable); // IMPORTANT, must update RenderGroups if they exist already
    void remove_renderable(Renderable* renderable);

    void traverse(TraverseCallback callback, uint64_t frame_id) const;

    uint32_t pass_count() const { return batches_.size(); }
    uint32_t group_count(Pass pass_number) const {
        if(pass_number >= batches_.size()) {
            throw std::out_of_range("Tried to access a pass that doesn't exist");
        }

        return batches_[pass_number].size();
    }

    void each_group(Pass pass, std::function<void (uint32_t, const RenderGroup&, const Batch&)> cb) {
        uint32_t i = 0;
        for(auto& batch: batches_[pass]){
            cb(i++, batch.first, batch.second);
        }
    }

private:
    // std::map is ordered, so by using the RenderGroup as the key we
    // minimize GL state changes (e.g. if a RenderGroupImpl orders by TextureID, then ShaderID
    // then we'll see  (TexID(1), ShaderID(1)), (TexID(1), ShaderID(2)) for example meaning the
    // texture doesn't change even if the shader does
    typedef std::map<RenderGroup, Batch> BatchMap;
    typedef std::vector<BatchMap> BatchPasses;

    Stage* stage_ = nullptr;
    RenderGroupFactory* render_group_factory_ = nullptr;
    BatchPasses batches_;

    sig::connection actor_created_;
    sig::connection actor_destroyed_;

    void clean_empty_batches();
};

}
}
