#pragma once

#include <vector>
#include <cstdint>
#include "../../vertex_data.h"
#include "../batching/render_queue.h"
#include "../../utils/vertex_lighting.h"

#ifdef __DREAMCAST__
#include <dc/pvr.h>
#endif

namespace smlt {

class PVRRenderer;

class PVRRenderQueueVisitor : public batcher::RenderQueueVisitor {
public:
    PVRRenderQueueVisitor(PVRRenderer* renderer, CameraPtr camera);

    void start_traversal(const batcher::RenderQueue& queue,
                         uint64_t frame_id,
                         StageNode* stage_node) override;

    void visit(const Renderable* renderable, const MaterialPass* pass,
               batcher::Iteration) override;

    void end_traversal(const batcher::RenderQueue& queue, StageNode* stage_node) override;

    void change_render_group(const batcher::RenderGroup* prev,
                             const batcher::RenderGroup* next) override;

    void change_material_pass(const MaterialPass* prev,
                              const MaterialPass* next) override;

    void apply_lights(const LightPtr* lights, const uint8_t count) override;

    static const int MAX_LIGHTS = 2;

    /* Number of distinct lights whose eye-space state we cache per frame.
     * apply_lights can be called once per renderable (and once per light for
     * ONCE_PER_LIGHT passes), but a light's eye-space position/direction is
     * constant for the whole camera pass, so we compute it once and reuse. */
    static const int LIGHT_CACHE_SIZE = 8;

private:
    PVRRenderer* renderer_;
    CameraPtr camera_;

    const MaterialPass* pass_ = nullptr;
    LightPtr light_;

#ifdef __DREAMCAST__
    pvr_poly_hdr_t poly_hdr_; /* Compiled polygon header — the current render state */
    /* Compiled modifier-volume headers — used when the active material pass
     * targets the modifier list. The two variants distinguish "more polys
     * to follow" from the closing poly of a volume. */
    pvr_mod_hdr_t mod_hdr_other_;
    pvr_mod_hdr_t mod_hdr_include_;
#endif

    /* Set in change_material_pass when the current pass targets the modifier
     * list, so do_visit can branch to the modifier-volume submission path. */
    bool emitting_modifier_volume_ = false;

    VertexLightState lights_[MAX_LIGHTS];
    float ambient_[3] = {0.2f, 0.2f, 0.2f};

    /* Per-frame cache of computed eye-space light state, keyed by Light*.
     * Reset in start_traversal; populated lazily by apply_lights. */
    struct LightCacheEntry {
        const Light* light = nullptr;
        VertexLightState state;
    };
    LightCacheEntry light_cache_[LIGHT_CACHE_SIZE];
    int light_cache_count_ = 0;
    VertexLightState light_scratch_; /* used when the cache overflows */

    /* Compute a light's frame-constant eye-space state (position/dir/colour/
     * intensity/range). Leaves state.enabled untouched. */
    void compute_light_state(LightPtr light, VertexLightState& out);

    /* Return cached eye-space state for `light`, computing and caching it on
     * first use this frame. */
    const VertexLightState& get_cached_light_state(LightPtr light);

    /* PBR material properties stored directly (no Phong conversion) */
    float mat_base_color_[4] = {1, 1, 1, 1};
    float mat_metallic_  = 0.0f;
    float mat_roughness_ = 0.4f;

    /* Cached affine UV transform derived from base_color_map_matrix(),
     * recomputed only on material-pass change. Mirrors GL1x's texture-matrix
     * behaviour (glLoadMatrixf on GL_TEXTURE) without touching the SH4's
     * single xmtrx register in the hot per-vertex loop, which stays loaded
     * with MVP/modelview for position transforms. Layout: u' = m_[0]*u +
     * m_[1]*v + m_[2]; v' = m_[3]*u + m_[4]*v + m_[5]. */
    bool  uv_matrix_identity_ = true;
    float uv_matrix_[6] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

    /* Reused scratch buffer that skinned renderables are posed into
     * immediately before submission - see skin_resolve.h. There's no
     * persistent GPU-side buffer on this backend (vertex data is streamed
     * straight to the Tile Accelerator every draw), so a single reused
     * buffer is all that's needed regardless of how many Armature instances
     * share a source mesh. */
    VertexData skin_scratch_{VertexSpecification()};

    void do_visit(const Renderable* renderable,
                  const MaterialPass* material_pass,
                  batcher::Iteration iteration);

};

} // namespace smlt
