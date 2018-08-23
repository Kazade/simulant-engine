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
#include "../nodes/camera.h"
#include "../nodes/actor.h"
#include "../nodes/light.h"
#include "../nodes/particle_system.h"
#include "../nodes/geom.h"

#include "frustum_partitioner.h"

namespace smlt {

void FrustumPartitioner::lights_and_geometry_visible_from(
        CameraID camera_id, std::vector<LightID> &lights_out,
        std::vector<StageNode*> &geom_out) {

    auto frustum = stage->camera(camera_id)->frustum();

    for(LightID lid: all_lights_) {
        auto light = stage->light(lid);
        auto aabb = light->transformed_aabb();
        if(light->type() == LIGHT_TYPE_DIRECTIONAL || frustum.intersects_aabb(aabb)) {
            lights_out.push_back(lid);
        }
    }

    for(ActorID eid: all_actors_) {
        auto actor = stage->actor(eid);

        if(frustum.intersects_aabb(actor->transformed_aabb())) {
            geom_out.push_back(actor);
        }
    }

    for(GeomID gid: all_geoms_) {
        auto geom = stage->geom(gid);
        if(frustum.intersects_aabb(geom->aabb())) {
            geom_out.push_back(geom);
        }
    }

    for(ParticleSystemID ps: all_particle_systems_) {
        auto system = stage->particle_system(ps);
        auto aabb = system->transformed_aabb();
        if(frustum.intersects_aabb(aabb)) {
            geom_out.push_back(system);
        }
    }
}

void FrustumPartitioner::apply_staged_write(const StagedWrite &write) {
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



}
