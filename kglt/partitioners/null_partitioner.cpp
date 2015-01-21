#include "../stage.h"
#include "../camera.h"
#include "../actor.h"
#include "../light.h"
#include "../particles.h"

#include "null_partitioner.h"

namespace kglt {

std::vector<LightID> NullPartitioner::lights_visible_from(CameraID camera_id) {
    auto frustum = stage()->window().camera(camera_id)->frustum();

    std::vector<LightID> result;
    for(LightID lid: all_lights_) {
        auto light = stage()->light(lid);
        AABB aabb = light->transformed_aabb();
        if(frustum.intersects_aabb(aabb)) {
            result.push_back(lid);
        }
    }

    return result;
}

std::vector<RenderablePtr> NullPartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<RenderablePtr> result;

    auto frustum = stage()->window().camera(camera_id)->frustum();

    //Just return all of the meshes in the stage
    for(ActorID eid: all_actors_) {
        auto subactors = stage()->actor(eid)->_subactors();

        for(auto ent: subactors) {
            if(frustum.intersects_aabb(ent->transformed_aabb())) {
                result.push_back(ent);
            }
        }
    }

    for(ParticleSystemID ps: all_particle_systems_) {
        auto system = stage()->particle_system(ps);
        AABB aabb = system->transformed_aabb();
        if(frustum.intersects_aabb(aabb)) {
            result.push_back(system.__object);
        }
    }

    return result;
}

}
