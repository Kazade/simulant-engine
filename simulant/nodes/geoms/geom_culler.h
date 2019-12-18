#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "../../types.h"


namespace smlt {

namespace batcher {
class RenderQueue;
}

struct Renderable;

/*
 * A GeomCuller is a class which compiles a mesh into some kind of internal representation
 * and then given a frustum will return a list of renderables for rendering.
 *
 * This is a second-step per-geom culling phase after the broadphase culling of the scen
 * partitioner.
 */

typedef std::shared_ptr<Renderable> RenderablePtr;
typedef std::vector<RenderablePtr> RenderableList;

typedef std::function<void (Renderable*)> EachRenderableCallback;

class Renderer;

class GeomCuller {
public:
    GeomCuller(Geom* geom, const MeshPtr mesh);
    virtual ~GeomCuller();

    bool is_compiled() const;

    void compile();
    void renderables_visible(const Frustum& frustum, batcher::RenderQueue* render_queue);

    void each_renderable(EachRenderableCallback cb);

    Geom* geom() const { return geom_; }
protected:
    Geom* geom_ = nullptr;
    MeshPtr mesh_;

private:
    bool compiled_ = false;

    virtual void _compile() = 0;
    virtual void _gather_renderables(const Frustum& frustum, batcher::RenderQueue* render_queue) = 0;
    virtual void _all_renderables(batcher::RenderQueue* rendre_queue) = 0;

    virtual const VertexData* _vertex_data() const = 0;

    friend class GeomCullerRenderable;

    /* We need to hold on to the materials that were attached to the source
     * mesh, otherwise they'll be garbage collected!
     */
    std::vector<MaterialPtr> material_refs_;
};

}
