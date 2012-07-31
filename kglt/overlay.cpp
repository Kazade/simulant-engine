#include <kazmath/mat4.h>
#include "overlay.h"
#include "scene.h"

namespace kglt {

Overlay::Overlay():
    zindex_(0) {

    set_ortho(-1.0, 1.0, -1.0, 1.0);
}

void Overlay::set_ortho(double left, double right, double bottom, double top) {
    kmMat4OrthographicProjection(&projection_matrix_, left, right, bottom, top, -1.0, 1.0);
}

/**
    Only allow the Scene to be a parent of this object
*/
bool Overlay::can_set_parent(Object* p) {
    return (bool) dynamic_cast<Scene*>(p);
}

}
