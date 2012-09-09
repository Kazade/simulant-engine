#include "../scene.h"
#include "null_partitioner.h"

namespace kglt {

std::vector<LightID> NullPartitioner::lights_within_range(const kmVec3& location) {
    std::vector<std::pair<LightID, float> > lights_in_range;

    //Find all the lights within range of the location
    for(LightID light_id: all_lights_) {
        Light& light = scene().light(light_id);

        kmVec3 diff;
        kmVec3Subtract(&diff, &location, &light.position());
        float dist = kmVec3Length(&diff);
        if(dist < light.range()) {
            lights_in_range.push_back(std::make_pair(light_id, dist));
        }
    }

    //Sort them by distance
    std::sort(lights_in_range.begin(), lights_in_range.end(),
              [](std::pair<LightID, float> lhs, std::pair<LightID, float> rhs) { return lhs.second < rhs.second; });

    //Return the LightIDs only
    std::vector<LightID> result;
    for(std::pair<LightID, float> p: lights_in_range) {
        result.push_back(p.first);
    }
    return result;
}

std::set<MeshID> NullPartitioner::meshes_visible_from(const Camera& camera) {
    //Just return all of the meshes
    return all_meshes_;
}

}
