/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at the
 * option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>

namespace smlt {

class VertexData;
class AABB;
struct Mat4;

/* Shared CPU skinning implementation. Blends `source`'s position/normal per
 * vertex against `joint_matrices` (armature_world_inverse * joint_world *
 * inverse_bind, one per joint - unused joint slots must be Mat4::zero() so
 * they can be blended in unconditionally) using the source's joint index /
 * weight vertex attributes, and writes the result into `dest`.
 *
 * `source` must have joint and weight vertex attributes. `dest` must already
 * be sized to source->count() and is written from its start via move_next();
 * everything but position/normal is left untouched, so callers that need the
 * remaining attributes (uv, colour, ...) must have copied them into `dest`
 * beforehand. `dest->done()` is called once skinning completes.
 *
 * This is the one true implementation of the pose math - both Armature (for
 * on-demand CPU readback) and every renderer backend (for skinning
 * immediately before submission, into a small reused scratch buffer rather
 * than a persistent per-instance copy) call this rather than duplicating it. */
void skin_vertices(const VertexData* source, const Mat4* joint_matrices,
                    uint16_t joint_count, VertexData* dest);

/* Position-only variant of skin_vertices(), used to keep an Armature's AABB
 * (needed for culling, so it must be known before it's decided whether a
 * mesh will be skinned for rendering at all) in step with the pose without
 * writing out a full posed vertex buffer. */
AABB skinned_aabb(const VertexData* source, const Mat4* joint_matrices,
                  uint16_t joint_count);

} // namespace smlt
