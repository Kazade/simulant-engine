//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../stage.h"
#include "../camera.h"
#include "../nodes/actor.h"
#include "../nodes/light.h"
#include "../nodes/particles.h"

#include "null_partitioner.h"

namespace smlt {

std::vector<LightID> NullPartitioner::lights_visible_from(CameraID camera_id) {
    auto frustum = stage->window->camera(camera_id)->frustum();

    std::vector<LightID> result;
    for(LightID lid: all_lights_) {
        auto light = stage->light(lid);
        auto aabb = light->bounds();
        if(light->type() == LIGHT_TYPE_DIRECTIONAL || frustum.intersects_aabb(aabb)) {
            result.push_back(lid);
        }
    }

    return result;
}

void NullPartitioner::apply_staged_write(const StagedWrite &write) {
    if(write.operation == WRITE_OPERATION_ADD) {
        if(write.actor_id) {
            all_actors_.insert(write.actor_id);
        } else if(write.geom_id) {
            all_geoms_.insert(write.geom_id);
        } else if(write.light_id) {
            all_lights_.insert(write.light_id);
        } else if(write.particle_system_id) {
            all_particle_systems_.insert(write.particle_system_id);
        }
    } else if(write.operation == WRITE_OPERATION_REMOVE) {
        if(write.actor_id) {
            all_actors_.erase(write.actor_id);
        } else if(write.geom_id) {
            all_geoms_.erase(write.geom_id);
        } else if(write.light_id) {
            all_lights_.erase(write.light_id);
        } else if(write.particle_system_id) {
            all_particle_systems_.erase(write.particle_system_id);
        }
    } else if(write.operation == WRITE_OPERATION_UPDATE) {
        // Do nothing!
    }
}

std::vector<RenderablePtr> NullPartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<RenderablePtr> result;

    auto frustum = stage->window->camera(camera_id)->frustum();

    //Just return all of the meshes in the stage
    for(ActorID eid: all_actors_) {
        auto actor = stage->actor(eid);
        auto subactors = actor->_subactors();

        for(auto ent: subactors) {
            if(frustum.intersects_aabb(ent->transformed_aabb())) {
                result.push_back(ent);
            }
        }
    }

    for(ParticleSystemID ps: all_particle_systems_) {
        auto system = stage->particle_system(ps);
        auto aabb = system->bounds();
        if(frustum.intersects_aabb(aabb)) {
            result.push_back(system->shared_from_this());
        }
    }

    return result;
}

}
