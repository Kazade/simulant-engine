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
#include "stage.h"

namespace smlt {

Geom::Geom(GeomID id, Stage* stage, MeshID mesh, const Vec3 &position, const Quaternion rotation):
    generic::Identifiable<GeomID>(id),
    Object(stage),
    Source(stage),
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

const AABB Geom::transformed_aabb() const {
    AABB box = aabb(); //Get the untransformed one

    auto pos = absolute_position();
    kmVec3Add(&box.min, &box.min, &pos);
    kmVec3Add(&box.max, &box.max, &pos);
    return box;
}

void Geom::ask_owner_for_destruction() {
    stage->delete_geom(id());
}

void Geom::compile() {

}

}
