#include "types.h"

namespace kglt {

static uint32_t counter = 0;

ShapeID get_next_shape_id() {
    return ShapeID(++counter);
}

}
