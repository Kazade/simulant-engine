/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILED_H
#define TILED_H

#include "../types.h"

namespace smlt {
namespace tiled {
    //Given a mesh and a pixel coordinate, returns the world coordinate using the tile width/height
    //stored when loading from a TMX file
    Vec2 pixel_to_world(const MeshPtr& mesh, const Vec2& pixel_position);

    float get_tile_render_size(const MeshPtr& mesh);
}
}

#endif // TILED_H
