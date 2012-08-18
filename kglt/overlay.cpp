#include <kazmath/mat4.h>
#include "overlay.h"
#include "scene.h"

namespace kglt {

Overlay::Overlay(Scene *scene, OverlayID id):
    Object(scene),
    generic::Identifiable<OverlayID>(id),
    zindex_(0) {

    set_ortho(-1.0, 1.0, -1.0, 1.0);
}

void Overlay::set_ortho(double left, double right, double bottom, double top) {
    kmMat4OrthographicProjection(&projection_matrix_, left, right, bottom, top, -1.0, 1.0);
}

}
