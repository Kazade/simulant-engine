#pragma once

#include "geom_culler.h"
#include "../../generic/tri_octree.h"

namespace smlt {

class OctreeCuller : public GeomCuller {
public:
    OctreeCuller(const MeshPtr mesh);

private:
    void _compile() override;
    void _gather_renderables(const Frustum &frustum, std::vector<std::shared_ptr<Renderable> > &out) override;
};

}
