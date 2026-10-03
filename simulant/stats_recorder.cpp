#include "meshes/mesh.h"
#include "stats_recorder.h"

namespace smlt {

void StatsRecorder::increment_polygons_rendered(MeshArrangement arrangement, uint32_t element_count) {
    polygons_rendered_ += polygon_count(arrangement, element_count);
}

}
