#include "../stage.h"
#include "../camera.h"
#include "../actor.h"
#include "../light.h"
#include "../particles.h"

#include "null_partitioner.h"

namespace kglt {

std::vector<LightID> NullPartitioner::lights_within_range(const Vec3& location) {
    std::vector<std::pair<LightID, float> > lights_in_range;

    //Find all the lights within range of the location
    for(LightID light_id: all_lights_) {
        Vec3 diff = location - stage()->light(light_id)->absolute_position();
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

std::vector<RenderablePtr> NullPartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<RenderablePtr> result;

    //Just return all of the meshes in the stage
    for(ActorID eid: all_actors_) {
        auto subactors = stage()->actor(eid)->_subactors();

        for(auto ent: subactors) {
            if(stage()->window().camera(camera_id)->frustum().intersects_aabb(ent->transformed_aabb())) {
                result.push_back(ent);
            }
        }
    }

    for(ParticleSystemID ps: all_particle_systems_) {
        auto system = stage()->particle_system(ps);
        AABB aabb = system->transformed_aabb();
        if(stage()->window().camera(camera_id)->frustum().intersects_aabb(aabb)) {
            result.push_back(system.__object);
        }
    }

    return result;
}

}
