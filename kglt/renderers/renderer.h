#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <vector>
#include <memory>

#include "../types.h"
#include "../generic/auto_weakptr.h"
#include "../window_base.h"

#include "batching/renderable.h"
#include "batching/render_queue.h"

namespace kglt {

class SubActor;

struct ElementRenderSpecification {
    MaterialID material_id;
    uint32_t count;
    uint8_t* indices; // Must be an array of UNSIGNED_SHORT
};

typedef std::vector<ElementRenderSpecification> ElementRenderList;

class Renderer:
    public batcher::RenderGroupFactory {

public:
    typedef std::shared_ptr<Renderer> ptr;

    Renderer(WindowBase* window):
        window_(window) {}

    virtual void render(
        CameraPtr camera,
        bool render_group_changed,
        const batcher::RenderGroup* render_group,
        Renderable* renderable,
        MaterialPass* material_pass,
        Light* light,
        const kglt::Colour& global_ambient,
        batcher::Iteration iteration
    ) = 0;

    Property<Renderer, WindowBase> window = { this, &Renderer::window_ };

    virtual void init_context() = 0;
    // virtual void upload_texture(Texture* texture) = 0;

private:    
    WindowBase* window_ = nullptr;
};

}

#endif // RENDERER_H
