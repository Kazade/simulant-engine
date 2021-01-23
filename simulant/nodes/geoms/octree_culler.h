#pragma once

#include <memory>
#include "geom_culler.h"
#include "../../vertex_data.h"

namespace smlt {

class RenderableFactory;

struct _OctreeCullerImpl;

class OctreeCuller : public GeomCuller {
public:
    OctreeCuller(Geom* geom, const MeshPtr mesh, uint8_t max_depth);

    AABB octree_bounds() const;

private:
    void _compile(const Vec3& pos, const Quaternion& rot, const Vec3& scale) override;
    void _gather_renderables(const Frustum &frustum, batcher::RenderQueue* render_queue) override;
    void _all_renderables(batcher::RenderQueue* queue) override;

    std::shared_ptr<_OctreeCullerImpl> pimpl_;

    IndexType index_type_ = INDEX_TYPE_16_BIT;
    uint8_t max_depth_;
};

}
