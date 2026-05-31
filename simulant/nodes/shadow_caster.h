#pragma once

#include <unordered_map>

#include "stage_node.h"
#include "../shadows.h"
#include "../assets/material.h"
#include "../vertex_data.h"

namespace smlt {

/*
 * ShadowCaster is a node that generates stencil shadow volumes for the
 * renderables produced by all of its descendant nodes (of any type). A
 * descendant contributes a shadow volume when its shadow_cast() is
 * SHADOW_CAST_ALWAYS. Lights are supplied by the render pipeline.
 *
 * Usage:
 *   auto caster = stage->create_child<ShadowCaster>();
 *   auto actor  = caster->create_child<Actor>(mesh);
 *   auto light  = stage->create_child<DirectionalLight>();
 *
 * On desktop (GL1/GL2) this uses the depth-pass stencil shadow volume method.
 * On Dreamcast (PVR) modifier volumes should be used instead (TODO).
 *
 * The scene's clear pass must clear the stencil buffer for correct results.
 *
 * Note: depth-pass only works correctly when the camera is outside all shadow
 * volumes. For a more robust but complex implementation, depth-fail (Carmack's
 * Reverse) with front/back caps can be added later.
 */
class ShadowCaster : public StageNode {
public:
    S_DEFINE_STAGE_NODE_META("shadow_caster");

    ShadowCaster(Scene* owner) : StageNode(owner, Meta::node_type) {}

protected:
    bool on_create(Params params) override;

private:
    bool do_generates_renderables_for_descendents() const override {
        return true;
    }

    void do_generate_renderables(batcher::RenderQueue* render_queue,
                                 const Camera* camera,
                                 const Viewport* viewport,
                                 const DetailLevel detail_level,
                                 Light** lights,
                                 const std::size_t light_count) override;

    void generate_shadow_geometry(const Renderable& renderable,
                                  const std::vector<EdgeInfo>& edges,
                                  LightPtr light,
                                  const Vec3& ext_dir_world,
                                  const Mat4& view_proj);

    /* Cached edge adjacency for persistent (key != -1) renderables. The
     * topology is transform- and deformation-invariant, so it's only rebuilt
     * when the index data changes. The face normals depend on vertex positions,
     * so they're refreshed whenever the vertex data (object or contents)
     * changes — cheap compared to a full rebuild, which is what lets animated
     * meshes share cached topology. */
    struct AdjacencyCacheEntry {
        std::vector<EdgeInfo> edges;
        const VertexData* vdata_ptr = nullptr;
        uint64_t vdata_stamp = 0;
        uint64_t idata_stamp = 0;
        uint64_t last_seen = 0; // generation in which this entry was last used
    };

    const std::vector<EdgeInfo>& adjacency_for(const Renderable& renderable);

    std::unordered_map<int64_t, AdjacencyCacheEntry> adjacency_cache_;
    std::vector<EdgeInfo> transient_adjacency_; // scratch for key == -1
    uint64_t cache_generation_ = 0;

    MaterialPtr sv_mat_incr_;     // GL: cull back, incr stencil on depth-pass
    MaterialPtr sv_mat_decr_;     // GL: cull front, decr stencil on depth-pass
    MaterialPtr overlay_mat_;     // GL: dark overlay where stencil != 0
    MaterialPtr sv_mat_modifier_; // PVR: cheap-shadow modifier volume

    std::unique_ptr<VertexData> sv_verts_;
    std::unique_ptr<IndexData>  sv_idx_;

    /* Per-volume slices of sv_idx_. Each entry is (first_index, index_count).
     * One entry per shadow caster × light pair, so PVR can submit each as its
     * own inclusion modifier volume — the hardware's per-volume parity is
     * reset on every closing INCLUDE_LAST poly, so overlapping shadows from
     * different casters correctly union rather than XOR. */
    std::vector<std::pair<uint32_t, uint32_t>> sv_volumes_;

    std::unique_ptr<VertexData> overlay_verts_;
    std::unique_ptr<IndexData>  overlay_idx_;

    Mat4 sv_identity_;     // Identity matrix — shadow verts are in world space
    Mat4 overlay_transform_; // Recomputed from camera each frame
};

} // namespace smlt
