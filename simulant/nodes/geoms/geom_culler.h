#pragma once

#include <vector>
#include <memory>
#include "../../types.h"

namespace smlt {

class Renderable;

/*
 * A GeomCuller is a class which compiles a mesh into some kind of internal representation
 * and then given a frustum will return a list of renderables for rendering.
 *
 * This is a second-step per-geom culling phase after the broadphase culling of the scen
 * partitioner.
 */

typedef std::vector<std::shared_ptr<Renderable>> RenderableList;

class GeomCuller {
public:
    GeomCuller(const MeshPtr mesh);

    bool is_compiled() const;

    void compile();
    RenderableList renderables_visible(const Frustum& frustum);

protected:
    MeshPtr mesh_;

private:
    bool compiled_ = false;

    virtual void _compile() = 0;
    virtual void _gather_renderables(const Frustum& frustum, std::vector<std::shared_ptr<Renderable>>& out) = 0;
};

}
