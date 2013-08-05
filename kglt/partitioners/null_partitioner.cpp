#include "../stage.h"
#include "../camera.h"
#include "../actor.h"
#include "../light.h"
#include "null_partitioner.h"

namespace kglt {

std::vector<LightID> NullPartitioner::lights_within_range(const Vec3& location) {
    std::vector<std::pair<LightID, float> > lights_in_range;

    //Find all the lights within range of the location
    for(LightID light_id: all_lights_) {
        Light& light = stage().light(light_id);

        Vec3 diff = location - light.absolute_position();
        float dist = diff.length();
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

std::vector<SubActor::ptr> NullPartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<SubActor::ptr> result;

    //Just return all of the meshes in the stage
    for(ActorID eid: all_actors_) {
        std::vector<SubActor::ptr> subactors = stage().actor(eid)->_subactors();

        for(SubActor::ptr ent: subactors) {
            if(stage().scene().camera(camera_id).frustum().intersects_aabb(ent->absolute_bounds())) {
                result.push_back(ent);
            }
        }
    }

    return result;
}

}
