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

#ifndef CAPSULE_H
#define CAPSULE_H

#include "../../meshes/mesh.h"

namespace smlt {
namespace procedural {
namespace mesh {

smlt::SubMesh* capsule(
    MeshPtr mesh,
    float diameter=0.5,
    float height=1.0,
    uint32_t segment_count=10,
    uint32_t vertical_segment_count=1,
    uint32_t ring_count=10,
    const smlt::Vec3& pos_offset=smlt::Vec3()
);

}
}
}

#endif // CAPSULE_H
