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
#include "geoms/quadtree_culler.h"
#include "camera.h"

namespace smlt {

Geom::Geom(Scene* owner):
    StageNode(owner, STAGE_NODE_TYPE_GEOM) {

}

bool Geom::on_create(void* params) {
    GeomParams* args = (GeomParams*) params;

    auto mesh_ptr = args->mesh;
    assert(mesh_ptr);

    if(!mesh_ptr) {
        return false;
    }

    if(args->options.type == GEOM_CULLER_TYPE_QUADTREE) {
        culler_.reset(new QuadtreeCuller(this, mesh_ptr, args->options.quadtree_max_depth));
    } else {
        assert(args->options.type == GEOM_CULLER_TYPE_OCTREE);
        culler_.reset(new OctreeCuller(this, mesh_ptr, args->options.octree_max_depth));
    }

    /* FIXME: Transform and recalc */
    aabb_ = mesh_ptr->aabb();

    culler_->compile(args->position, args->rotation, args->scale);
    return true;
}

const AABB &Geom::aabb() const {
    return aabb_;
}

void Geom::do_generate_renderables(batcher::RenderQueue* render_queue, const Camera* camera, const Viewport* , const DetailLevel detail_level) {
    _S_UNUSED(detail_level);

    culler_->renderables_visible(camera->frustum(), render_queue);
}

}
