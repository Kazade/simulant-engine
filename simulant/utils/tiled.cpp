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


#include "tiled.h"
#include "../meshes/mesh.h"

namespace smlt {
namespace tiled {

Vec2 pixel_to_world(const MeshPtr& mesh, const Vec2& pixel_position) {
    //FIXME: If the mesh has been scaled this won't work...

    int map_tile_width = mesh->data->get<int>("TILED_MAP_TILE_WIDTH");
    int map_tile_height = mesh->data->get<int>("TILED_MAP_TILE_HEIGHT");
    float render_size = get_tile_render_size(mesh);

    return Vec2(
        ((pixel_position.x / float(map_tile_width)) * render_size),
        ((pixel_position.y / float(map_tile_height)) * render_size)
    );
}

float get_tile_render_size(const MeshPtr& mesh) {
    return mesh->data->get<float>("TILED_TILE_RENDER_SIZE");
}

}
}
