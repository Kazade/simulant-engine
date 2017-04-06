#include "collider.h"

namespace smlt {
namespace controllers {

std::pair<Vec3, Vec3> calculate_bounds(const std::vector<Vec3>& vertices) {
    float min = std::numeric_limits<float>::max(), max = std::numeric_limits<float>::lowest();

    for(auto& vertex: vertices) {
        if(vertex.x < min) min = vertex.x;
        if(vertex.y < min) min = vertex.y;
        if(vertex.z < min) min = vertex.z;
        if(vertex.x > max) max = vertex.x;
        if(vertex.y > max) max = vertex.y;
        if(vertex.z > max) max = vertex.z;
    }

    return std::make_pair(Vec3(min, min, min), Vec3(max, max, max));
}


}
}
