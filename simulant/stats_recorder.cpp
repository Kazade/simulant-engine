#include "meshes/mesh.h"
#include "stats_recorder.h"

namespace smlt {

void StatsRecorder::increment_polygons_rendered(MeshArrangement arrangement, uint32_t element_count) {
    uint32_t increment = 0;

    switch(arrangement) {
    case MESH_ARRANGEMENT_TRIANGLES:
        increment = element_count / 3;
    break;
    case MESH_ARRANGEMENT_TRIANGLE_STRIP:
        increment = element_count - 2;
    break;
    case MESH_ARRANGEMENT_TRIANGLE_FAN:
        increment = element_count - 2;
    break;
    case MESH_ARRANGEMENT_LINES:
        increment = element_count / 2;
    break;
    case MESH_ARRANGEMENT_LINE_STRIP:
        increment = element_count - 1;
    break;
    case MESH_ARRANGEMENT_QUADS:
        increment = element_count / 4;
    break;
    }

    polygons_rendered_ += increment;
}

}
