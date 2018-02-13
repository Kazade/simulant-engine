#pragma once

#include <memory>
#include "geom_culler.h"
#include "../../generic/tri_octree.h"
#include "../../vertex_data.h"

namespace smlt {

struct _OctreeCullerImpl;

class OctreeCuller : public GeomCuller {
public:
    OctreeCuller(Geom* geom, const MeshPtr mesh);

private:
    void _compile() override;
    void _gather_renderables(const Frustum &frustum, std::vector<std::shared_ptr<Renderable> > &out) override;

    std::shared_ptr<_OctreeCullerImpl> pimpl_;

    VertexData vertices_;
    IndexType index_type_ = INDEX_TYPE_16_BIT;
};

}
