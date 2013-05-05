#include "../subscene.h"
#include "../camera.h"
#include "../entity.h"
#include "../light.h"
#include "null_partitioner.h"

namespace kglt {

std::vector<LightID> NullPartitioner::lights_within_range(const kmVec3& location) {
    std::vector<std::pair<LightID, float> > lights_in_range;

    //Find all the lights within range of the location
    for(LightID light_id: all_lights_) {
        Light& light = subscene().light(light_id);

        kmVec3 diff;
        kmVec3Subtract(&diff, &location, &light.position());
        float dist = kmVec3Length(&diff);
        //if(dist < light.range()) {
            lights_in_range.push_back(std::make_pair(light_id, dist));
        //}
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

std::vector<SubEntity::ptr> NullPartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<SubEntity::ptr> result;

    //Just return all of the meshes in the subscene
    for(EntityID eid: all_entities_) {
        std::vector<SubEntity::ptr> subentities = subscene().entity(eid)._subentities();

        for(SubEntity::ptr ent: subentities) {
            if(subscene().camera(camera_id).frustum().intersects_aabb(ent->absolute_bounds())) {
                result.push_back(ent);
            }
        }
    }

    return result;
}

}
