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

#ifndef PARENT_SETTER_MIXIN_H
#define PARENT_SETTER_MIXIN_H

#include "../types.h"

namespace smlt {

//Subclassing this allows us to set parents using
//IDs, rather than references which makes for a safer API
template<typename T>
class ParentSetterMixin : public T {
public:
    ParentSetterMixin(Stage* stage):
        T(stage) {}

    template<typename ...Args>
    ParentSetterMixin(Stage* stage, Args&& ...args):
        T(stage, std::forward<Args>(args)...){

    }

    void set_parent(ActorID actor) {
        T::set_parent(T::stage->actor(actor));
    }

    void set_parent(ActorPtr actor) {
        T::set_parent(actor);
    }

    void set_parent(LightID light) {
        T::set_parent(T::stage->light(light));
    }

    void set_parent(LightPtr light) {
        T::set_parent(light);
    }

    void set_parent(CameraID camera) {
        T::set_parent(T::stage->camera(camera));
    }

    void set_parent(CameraPtr camera) {
        T::set_parent(camera);
    }

    void set_parent(SpriteID sprite) {
        T::set_parent(T::stage->sprite(sprite));
    }

    void set_parent(SpritePtr sprite) {
        T::set_parent(sprite);
    }

    void set_parent(ParticleSystemID particles) {
        T::set_parent(T::stage->particle_system(particles));
    }

    void set_parent(ParticleSystemPtr particle_system) {
        T::set_parent(particle_system);
    }

protected:
    using T::set_parent;

    friend class Stage;
};

}
#endif // PARENT_SETTER_MIXIN_H
