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

#ifndef RECTANGLE_H_INCLUDED
#define RECTANGLE_H_INCLUDED

#include "../../meshes/mesh.h"

namespace smlt {

class Mesh;

namespace procedural {
namespace mesh {

SubMesh* new_rectangle_submesh(MeshPtr mesh, MaterialPtr mat,
    float width,
    float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0);

SubMesh* rectangle(MeshPtr mesh, MaterialPtr material,
    float width, float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true);

smlt::SubMesh *rectangle_outline(
    MeshPtr mesh,
    MaterialPtr material,
    float width, float height,
    float x_offset=0.0, float y_offset=0.0, float z_offset=0.0,
    bool clear=true);

}
}
}


#endif // RECTANGLE_H_INCLUDED
