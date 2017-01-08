/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NULL_PARTITIONER_H
#define NULL_PARTITIONER_H

#include "../partitioner.h"

namespace smlt {

class SubActor;

class NullPartitioner : public Partitioner {
public:
    NullPartitioner(Stage* ss):
        Partitioner(ss) {}

    void add_actor(ActorID obj) {
        all_actors_.insert(obj);
    }

    void remove_actor(ActorID obj) {
        all_actors_.erase(obj);
    }

    void add_geom(GeomID geom_id) {
        all_geoms_.insert(geom_id);
    }

    void remove_geom(GeomID geom_id) {
        all_geoms_.erase(geom_id);
    }

    void add_light(LightID obj) {
        all_lights_.insert(obj);
    }

    void remove_light(LightID obj) {
        all_lights_.erase(obj);
    }

    void add_particle_system(ParticleSystemID ps) {
        all_particle_systems_.insert(ps);
    }

    void remove_particle_system(ParticleSystemID ps) {
        all_particle_systems_.erase(ps);
    }

    std::vector<LightID> lights_visible_from(CameraID camera_id);
    std::vector<std::shared_ptr<Renderable>> geometry_visible_from(CameraID camera_id);

private:
    std::set<ParticleSystemID> all_particle_systems_;
    std::set<ActorID> all_actors_;
    std::set<GeomID> all_geoms_;
    std::set<LightID> all_lights_;
};

}

#endif // NULL_PARTITIONER_H
