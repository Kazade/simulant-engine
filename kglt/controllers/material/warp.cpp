
#include "./warp.h"

namespace kglt {
namespace controllers {
namespace material {

void Warp::do_update(double dt) {
    auto& pass = material->pass(0);
    auto& tex_unit = pass.texture_unit(0);

    tex_unit.matrix().mat[0] = 1.0 + (sin(time_) * 0.25);
    tex_unit.matrix().mat[5] = 1.0 + (sin(time_) * 0.25);

    time_ += dt * 0.25;
}

}
}
}

