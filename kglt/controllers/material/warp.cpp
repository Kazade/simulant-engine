
#include "./warp.h"

namespace kglt {
namespace controllers {
namespace material {

void Warp::do_update(double dt) {
    auto& pass = material->pass(0);
    auto& tex_unit = pass.texture_unit(0);

    tex_unit.matrix().mat[0] = sin(time_ * 5.0) + 0.5;
    tex_unit.matrix().mat[5] = cos(time_ * 5.0);

    time_ += (dt * 0.125);
}

}
}
}

