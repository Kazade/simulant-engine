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

#include "geom.h"
#include "../stage.h"

namespace smlt {

Geom::Geom(GeomID id, Stage* stage, SoundDriver* sound_driver, MeshID mesh, const Vec3 &position, const Quaternion rotation):
    StageNode(stage),
    generic::Identifiable<GeomID>(id),
    Source(stage, sound_driver),
    render_priority_(RENDER_PRIORITY_MAIN) {

    set_parent(stage);

    mesh_ = stage->assets->mesh(mesh)->shared_from_this();

    compile();
}

VertexData* Geom::get_shared_data() const {
    return mesh_->shared_data.get();
}

const AABB Geom::aabb() const {
    return mesh_->aabb();
}

void Geom::ask_owner_for_destruction() {
    stage->delete_geom(id());
}

void Geom::compile() {

}

}
