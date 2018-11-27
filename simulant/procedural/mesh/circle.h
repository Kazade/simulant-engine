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

#ifndef CIRCLE_H
#define CIRCLE_H

#include "../../meshes/mesh.h"

namespace smlt {
namespace procedural {
namespace mesh {

SubMesh* circle(
    smlt::Mesh& mesh,
    float diameter=1.0,
    int32_t point_count=20,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0
);

SubMesh* circle_outline(
    smlt::Mesh& mesh,
    float diameter=1.0,
    int32_t point_count=20,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0
);

}
}
}

#endif // CIRCLE_H
