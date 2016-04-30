#pragma once

#include "../renderer.h"

namespace kglt {
namespace new_batcher {

class MaterialPass;

class Batch {
public:
    void add_renderable(Renderable* renderable);

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

    virtual ~RenderGroup() {}
    virtual bool operator<(const RenderGroup& rhs) const = 0;
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

class RenderQueue {
public:
    typedef std::function<void (Renderable*, MaterialPass*, uint32_t iteration)> TraverseCallback;

    RenderQueue(Stage* stage);

    void insert_renderable(Renderable* renderable); // IMPORTANT, must updated RenderGroups if they exist already
    void remove_renderable(Renderable* renderable);

    void traverse(TraverseCallback callback) const;

private:
    typedef std::map<RenderGroup, Batch> BatchMap;

    BatchMap batches_;
};

}
}
