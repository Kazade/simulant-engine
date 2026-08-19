#pragma once

#include "../../utils/skinning.h"
#include "../../vertex_data.h"
#include "renderable.h"

namespace smlt {

/* For renderer backends with no persistent GPU-side vertex buffer (GL1x,
 * PSP, PVR): these read straight from a Renderable's vertex_data every draw
 * call, which is the natural point to skin - into a single small buffer
 * reused across every skinned draw, rather than each Armature instance
 * keeping a permanent posed copy alive.
 *
 * Returns `renderable->vertex_data` unchanged if it doesn't need skinning.
 * Otherwise copies it into `scratch` (resetting its specification if
 * necessary) and skins it in place, returning `&scratch`. `scratch` is
 * expected to be a member of the caller (e.g. a RenderQueueVisitor) so its
 * backing storage is reused rather than reallocated every call. */
inline const VertexData* resolve_vertex_data(const Renderable* renderable,
                                              VertexData& scratch) {
    if(!renderable->skinning) {
        return renderable->vertex_data;
    }

    const VertexData* source = renderable->vertex_data;

    if(scratch.vertex_specification() != source->vertex_specification()) {
        scratch.reset(source->vertex_specification());
    }

    source->clone_into(scratch);
    skin_vertices(source, renderable->skinning->joint_matrices,
                  renderable->skinning->joint_count, &scratch);

    return &scratch;
}

} // namespace smlt
