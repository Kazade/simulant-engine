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

/* Row-pointer variant of skin_blend_matrix(), for callers that already have
 * a raw pointer to the vertex row rather than a VertexData+index pair - e.g.
 * a renderer walking rows directly by stride, one attribute-conversion
 * function shared between an indexed and a non-indexed (range) submission
 * path. `joint_offset`/`weight_offset` are
 * source->vertex_specification().joint_offset()/weight_offset() (declared
 * as int16_t there as AttributeOffset). Same blend/return semantics as
 * skin_blend_matrix(). */
bool skin_blend_matrix_row(const uint8_t* row, int16_t joint_offset,
                           int16_t weight_offset, const Mat4* joint_matrices,
                           uint16_t joint_count, bool use_byte_joints);

/* Combines skin_blend_matrix_row() with the position/normal transform, for
 * callers that don't otherwise need to touch XMTRX themselves (i.e.
 * everything except PVR, which chains a further CPU-side MVP/modelview
 * transform of its own and so blends in place - see pvr_render_queue_visitor
 * .cpp). `pos_in`/`normal_in` (3 floats each, either may be null to skip)
 * are transformed into `pos_out`/`normal_out` (normal is also
 * re-normalized). Returns false (leaving the outputs untouched) if the
 * vertex has no weight - callers should use the bind-pose values unchanged
 * in that case. */
bool skin_vertex_row(const uint8_t* row, int16_t joint_offset,
                     int16_t weight_offset, const Mat4* joint_matrices,
                     uint16_t joint_count, bool use_byte_joints,
                     const float* pos_in, float* pos_out,
                     const float* normal_in, float* normal_out);

/* Loads vertex `vertex_index`'s joint indices/weights from `source` and
 * leaves XMTRX holding the blended skinning matrix
 * (armature_world_inverse * joint_world * inverse_bind, blended by weight -
 * unused joint slots must be Mat4::zero() so they can be blended in
 * unconditionally). `use_byte_joints` should be
 * `source->vertex_specification().joint_attribute == VERTEX_ATTRIBUTE_4UB`,
 * hoisted out of the caller's loop rather than re-read per vertex.
 *
 * Returns false (XMTRX left untouched) if the vertex has no weight at all
 * (not every format weights every vertex - MS3D in particular allows
 * unweighted vertices), in which case callers should fall back to the
 * bind-pose position/normal unchanged.
 *
 * This is the primitive for renderer backends that stream vertices through
 * their own transform/clip loop (PVR, PSP) and want to skin in place -
 * reading straight off the row they're already walking - rather than
 * pre-populate a whole posed buffer. See skin_vertices() for the
 * whole-buffer equivalent. */
bool skin_blend_matrix(const VertexData* source, uint32_t vertex_index,
                       const Mat4* joint_matrices, uint16_t joint_count,
                       bool use_byte_joints);

/* Shared CPU skinning implementation. Blends `source`'s position/normal per
 * vertex against `joint_matrices` (see skin_blend_matrix()) using the
 * source's joint index/weight vertex attributes, and writes the result into
 * `dest`, for vertices in [range_start, range_end) only (defaults to the
 * whole buffer).
 *
 * `source` must have joint and weight vertex attributes. `dest` must already
 * be sized to at least range_end; only [range_start, range_end) is touched,
 * so a caller that only needs a subset (e.g. a renderer bounding this to a
 * submesh's active index range) can pass a `dest` no bigger than that
 * subset - the position/normal *and* the source index numbering are both
 * preserved, so e.g. an index buffer that already references absolute
 * vertex numbers keeps working against `dest` unmodified. Everything but
 * position/normal is left untouched, so callers that need the remaining
 * attributes (uv, colour, ...) must have copied them into `dest`
 * beforehand. `dest->done()` is called once skinning completes.
 *
 * Intended for backends with no per-vertex transform loop to fuse into (GL1x,
 * and GL2's upload path) - they skin into a small reused buffer immediately
 * before submission rather than a persistent per-instance copy. PVR/PSP
 * don't use this - see skin_blend_matrix(). */
void skin_vertices(const VertexData* source, const Mat4* joint_matrices,
                    uint16_t joint_count, VertexData* dest,
                    uint32_t range_start = 0,
                    uint32_t range_end = UINT32_MAX);

/* Position-only variant of skin_vertices(), used to keep an Armature's AABB
 * (needed for culling, so it must be known before it's decided whether a
 * mesh will be skinned for rendering at all) in step with the pose without
 * writing out a full posed vertex buffer. */
AABB skinned_aabb(const VertexData* source, const Mat4* joint_matrices,
                  uint16_t joint_count);

} // namespace smlt
