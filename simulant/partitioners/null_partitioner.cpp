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
#include "../actor.h"
#include "../light.h"
#include "../particles.h"

#include "null_partitioner.h"

namespace smlt {

std::vector<LightID> NullPartitioner::lights_visible_from(CameraID camera_id) {
    auto frustum = stage->window->camera(camera_id)->frustum();

    std::vector<LightID> result;
    for(LightID lid: all_lights_) {
        auto light = stage->light(lid);
        AABB aabb = light->transformed_aabb();
        if(light->type() == LIGHT_TYPE_DIRECTIONAL || frustum.intersects_aabb(aabb)) {
            result.push_back(lid);
        }
    }

    return result;
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
        AABB aabb = system->transformed_aabb();
        if(frustum.intersects_aabb(aabb)) {
            result.push_back(system->shared_from_this());
        }
    }

    return result;
}

}
