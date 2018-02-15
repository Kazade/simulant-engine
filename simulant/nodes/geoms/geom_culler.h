#pragma once

#include <vector>
#include <memory>
#include "../../types.h"

namespace smlt {

class HardwareBuffer;
class Renderable;

/*
 * A GeomCuller is a class which compiles a mesh into some kind of internal representation
 * and then given a frustum will return a list of renderables for rendering.
 *
 * This is a second-step per-geom culling phase after the broadphase culling of the scen
 * partitioner.
 */

typedef std::shared_ptr<Renderable> RenderablePtr;
typedef std::vector<RenderablePtr> RenderableList;

class Renderer;

class GeomCuller {
public:
    GeomCuller(Geom* geom, const MeshPtr mesh);

    bool is_compiled() const;

    void compile();
    RenderableList renderables_visible(const Frustum& frustum);

protected:
    Geom* geom_ = nullptr;
    MeshPtr mesh_;

private:
    bool compiled_ = false;

    virtual void _prepare_buffers(Renderer* renderer) = 0;
    virtual void _compile() = 0;
    virtual void _gather_renderables(const Frustum& frustum, std::vector<std::shared_ptr<Renderable>>& out) = 0;

    virtual const VertexData* _vertex_data() const = 0;
    virtual HardwareBuffer* _vertex_attribute_buffer() const = 0;

    friend class GeomCullerRenderable;
};

}
