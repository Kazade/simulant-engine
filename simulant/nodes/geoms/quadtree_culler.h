#pragma once

#include <memory>
#include "geom_culler.h"
#include "../../vertex_data.h"

namespace smlt {

class RenderableFactory;

struct _QuadtreeCullerImpl;

class QuadtreeCuller : public GeomCuller {
public:
    QuadtreeCuller(Geom* geom, const MeshPtr mesh, uint8_t max_depth);

    AABB Quadtree_bounds() const;

    const VertexData* _vertex_data() const override;
private:
    void _compile() override;
    void _gather_renderables(const Frustum &frustum, RenderableFactory* factory) override;
    void _all_renderables(RenderableFactory* factory) override;

    std::shared_ptr<_QuadtreeCullerImpl> pimpl_;

    IndexType index_type_ = INDEX_TYPE_16_BIT;
    uint8_t max_depth_;
};

}
