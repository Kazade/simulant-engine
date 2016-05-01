#pragma once

#include <list>
#include "./renderable.h"

namespace kglt {

class MaterialPass;
class Renderer;

namespace new_batcher {

class Batch {
public:
    void add_renderable(Renderable* renderable) {
        renderables_.push_back(renderable);
    }

    void each(std::function<void (uint32_t, Renderable*)> func) const {
        uint32_t i = 0;
        for(auto& renderable: renderables_) {
            func(i++, renderable);
        }
    }

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

    virtual ~RenderGroupImpl() {}

    bool operator<(const RenderGroupImpl& rhs) {
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
    RenderGroup(RenderGroupImpl::ptr impl):
        impl_(impl) {}

    bool operator<(const RenderGroup& rhs) const {
        return impl_ && rhs.impl_ && *impl_ < *rhs.impl_;
    }

private:
    RenderGroupImpl::ptr impl_;
};


class RenderGroupFactory {
public:
    virtual ~RenderGroupFactory() {}

    virtual RenderGroup new_render_group(MaterialPass* material_pass) = 0;
};

typedef uint32_t Pass;
typedef uint32_t Iteration;

class RenderQueue {
public:
    typedef std::function<void (const RenderGroup*, const RenderGroup*, Renderable*, MaterialPass*, Iteration)> TraverseCallback;

    RenderQueue(Stage* stage, RenderGroupFactory* render_group_factory);

    void insert_renderable(Renderable* renderable); // IMPORTANT, must updated RenderGroups if they exist already
    void remove_renderable(Renderable* renderable);

    void traverse(TraverseCallback callback) const;

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
};

}
}
