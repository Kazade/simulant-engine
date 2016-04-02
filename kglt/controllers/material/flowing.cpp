
#include "./flowing.h"

namespace kglt {
namespace controllers {
namespace material {

void Flowing::do_update(double dt) {
    float scroll = -(time_ - int(time_));

    auto& pass = material->pass(0);
    auto& tex_unit = pass.texture_unit(0);

    tex_unit.matrix().mat[12] = scroll;
    tex_unit.matrix().mat[13] = sin(time_ * 5.0);

    time_ += (dt * 0.125);
}

}
}
}
