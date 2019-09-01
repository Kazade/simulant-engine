//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "geom.h"
#include "../stage.h"
#include "geoms/octree_culler.h"
#include "camera.h"
#include "../renderers/batching/renderable_store.h"

namespace smlt {

Geom::Geom(GeomID id, Stage* stage, SoundDriver* sound_driver, MeshID mesh, const Vec3 &position, const Quaternion rotation, GeomCullerOptions culler_options):
    StageNode(stage),
    generic::Identifiable<GeomID>(id),
    Source(stage, sound_driver),
    mesh_id_(mesh),
    render_priority_(RENDER_PRIORITY_MAIN),
    culler_options_(culler_options) {

    set_parent(stage);
}

bool Geom::init() {
    auto mesh_ptr = stage->assets->mesh(mesh_id_);
    assert(mesh_ptr);

    if(!mesh_ptr) {
        return false;
    }

    if(culler_options_.type == GEOM_CULLER_TYPE_QUADTREE) {
        assert(0 && "Not yet implemented");
    } else {
        culler_.reset(new OctreeCuller(this, mesh_ptr, culler_options_.octree_max_depth));
    }

    /* FIXME: Transform and recalc */
    aabb_ = mesh_ptr->aabb();

    culler_->compile();
    return true;
}

const AABB &Geom::aabb() const {
    return aabb_;
}

void Geom::destroy() {
    stage->destroy_geom(id());
}

void Geom::_get_renderables(RenderableFactory* factory, CameraPtr camera, DetailLevel detail_level) {
    culler_->renderables_visible(camera->frustum(), factory);
}


}
