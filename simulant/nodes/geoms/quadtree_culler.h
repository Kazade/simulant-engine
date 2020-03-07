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

private:
    void _compile(const Vec3 &pos, const Quaternion &rot) override;
    void _gather_renderables(const Frustum &frustum, batcher::RenderQueue* render_queue) override;
    void _all_renderables(batcher::RenderQueue* render_queue) override;

    std::shared_ptr<_QuadtreeCullerImpl> pimpl_;

    IndexType index_type_ = INDEX_TYPE_16_BIT;
    uint8_t max_depth_;
};

}
