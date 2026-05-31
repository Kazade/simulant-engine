#include "shadow_caster.h"

#include "light.h"
#include "camera.h"
#include "../scenes/scene.h"
#include "../asset_manager.h"
#include "../renderers/batching/render_queue.h"
#include "../renderers/batching/renderable.h"
#include "../shadows.h"

namespace smlt {

static constexpr float SHADOW_EXTRUSION_DISTANCE = 100.0f;

bool ShadowCaster::on_create(Params params) {
    if(!StageNode::on_create(params)) {
        return false;
    }

    // Pass 1: cull back faces (draw front), increment stencil on depth pass
    sv_mat_incr_ = scene->assets->clone_default_material(GARBAGE_COLLECT_NEVER);
    sv_mat_incr_->set_pass_count(1);
    {
        auto* p = sv_mat_incr_->pass(0);
        p->set_cull_mode(CULL_MODE_BACK_FACE);
        p->set_depth_write_enabled(false);
        p->set_depth_test_enabled(true);
        p->set_lighting_enabled(false);
        p->set_textures_enabled(0);
        p->set_color_write_enabled(false);
        p->set_stencil_test_enabled(true);
        p->set_stencil_func(STENCIL_FUNC_ALWAYS);
        p->set_stencil_ops(STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_INCR_WRAP);
    }

    // Pass 2: cull front faces (draw back), decrement stencil on depth pass
    sv_mat_decr_ = scene->assets->clone_default_material(GARBAGE_COLLECT_NEVER);
    sv_mat_decr_->set_pass_count(1);
    {
        auto* p = sv_mat_decr_->pass(0);
        p->set_cull_mode(CULL_MODE_FRONT_FACE);
        p->set_depth_write_enabled(false);
        p->set_depth_test_enabled(true);
        p->set_lighting_enabled(false);
        p->set_textures_enabled(0);
        p->set_color_write_enabled(false);
        p->set_stencil_test_enabled(true);
        p->set_stencil_func(STENCIL_FUNC_ALWAYS);
        p->set_stencil_ops(STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_DECR_WRAP);
    }

    // PVR: cheap-shadow modifier volume. The PVR renderer routes polygons
    // tagged with POLYGON_LIST_TARGET_MODIFIER to its modifier list; GL
    // renderers skip the pass at the visitor level.
    sv_mat_modifier_ = scene->assets->clone_default_material(GARBAGE_COLLECT_NEVER);
    sv_mat_modifier_->set_pass_count(1);
    {
        auto* p = sv_mat_modifier_->pass(0);
        p->set_cull_mode(CULL_MODE_NONE);
        p->set_depth_write_enabled(false);
        p->set_depth_test_enabled(true);
        p->set_lighting_enabled(false);
        p->set_textures_enabled(0);
        p->set_color_write_enabled(false);
        p->set_blend_func(BLEND_NONE);
        p->set_polygon_list_target(POLYGON_LIST_TARGET_MODIFIER);
    }

    // Overlay: dark semi-transparent quad where stencil != 0
    overlay_mat_ = scene->assets->clone_default_material(GARBAGE_COLLECT_NEVER);
    overlay_mat_->set_pass_count(1);
    {
        auto* p = overlay_mat_->pass(0);
        p->set_cull_mode(CULL_MODE_NONE);
        p->set_depth_write_enabled(false);
        p->set_depth_test_enabled(false);
        p->set_lighting_enabled(false);
        p->set_textures_enabled(0);
        p->set_blend_func(BLEND_ALPHA);
        p->set_base_color(Color(0.0f, 0.0f, 0.0f, 0.5f));
        p->set_stencil_test_enabled(true);
        p->set_stencil_func(STENCIL_FUNC_NOT_EQUAL, 0);
        p->set_stencil_ops(STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_KEEP);
    }

    // Dynamic shadow volume geometry (rebuilt each frame)
    sv_verts_ = std::make_unique<VertexData>(VertexSpecification::POSITION_ONLY);
    sv_idx_   = std::make_unique<IndexData>(INDEX_TYPE_16_BIT);

    // Full-screen triangle for the shadow overlay
    // Vertices in NDC: combined with overlay_transform_ they cover the whole screen.
    // The dark colour is baked into the vertices (not just base_color) because the
    // fixed-function (gl1x) path uses the vertex colour directly for unlit geometry;
    // relying on base_color alone leaves it white there.
    overlay_verts_ = std::make_unique<VertexData>(VertexSpecification::POSITION_AND_DIFFUSE);
    overlay_idx_   = std::make_unique<IndexData>(INDEX_TYPE_8_BIT);

    const Color overlay_color(0.0f, 0.0f, 0.0f, 0.5f);
    overlay_verts_->position(Vec3(-1.0f, -1.0f, 0.0f)); overlay_verts_->color(overlay_color); overlay_verts_->move_next();
    overlay_verts_->position(Vec3( 3.0f, -1.0f, 0.0f)); overlay_verts_->color(overlay_color); overlay_verts_->move_next();
    overlay_verts_->position(Vec3(-1.0f,  3.0f, 0.0f)); overlay_verts_->color(overlay_color); overlay_verts_->move_next();
    overlay_verts_->done();

    overlay_idx_->index(0); overlay_idx_->index(1); overlay_idx_->index(2);
    overlay_idx_->done();

    // sv_identity_ is already identity-initialised by Mat4 default constructor

    return true;
}

const std::vector<EdgeInfo>& ShadowCaster::adjacency_for(const Renderable& r) {
    if(r.key != -1) {
        AdjacencyCacheEntry& entry = adjacency_cache_[r.key];

        const uint64_t vstamp = r.vertex_data->last_updated();
        const uint64_t istamp = r.index_data ? r.index_data->last_updated() : 0;

        const bool fresh = (entry.last_seen == 0);

        // Topology only changes if the index data changes (the connectivity is
        // invariant under transform and deformation). A full rebuild also
        // produces fresh normals.
        if(fresh || entry.idata_stamp != istamp) {
            build_silhouette_adjacency(r.vertex_data, r.index_data,
                                       r.index_element_count, r.arrangement,
                                       entry.edges);
            entry.idata_stamp = istamp;
        }
        // Normals depend on the vertex positions, so refresh them if the vertex
        // data object differs (e.g. another animated instance of the same mesh)
        // or its contents changed (e.g. a new animation frame).
        else if(entry.vdata_ptr != r.vertex_data || entry.vdata_stamp != vstamp) {
            recompute_silhouette_normals(r.vertex_data, entry.edges);
        }

        entry.vdata_ptr = r.vertex_data;
        entry.vdata_stamp = vstamp;
        entry.last_seen = cache_generation_;
        return entry.edges;
    }

    // Transient renderable: rebuild into the reused scratch buffer each frame.
    build_silhouette_adjacency(r.vertex_data, r.index_data,
                               r.index_element_count, r.arrangement,
                               transient_adjacency_);
    return transient_adjacency_;
}

void ShadowCaster::generate_shadow_geometry(const Renderable& renderable,
                                             const std::vector<EdgeInfo>& edges,
                                             LightPtr light,
                                             const Vec3& ext_dir_world,
                                             const Mat4& view_proj) {
    if(!renderable.vertex_data || edges.empty()) {
        return;
    }

    const Mat4& world_mat = (renderable.final_transformation)
                                ? *renderable.final_transformation
                                : sv_identity_;

    MeshSilhouette silhouette(renderable.vertex_data, edges, world_mat, light);
    const auto& sil_edges = silhouette.edge_list();
    if(sil_edges.empty()) {
        return;
    }

    /* Per-unit-extrusion rate of change in the clip-space near-plane test
     * value (z + w). If the extrusion direction has a component toward the
     * camera this is negative, and we'll clamp the per-vertex distance so
     * the extruded vertex never crosses the near plane — that keeps the
     * shadow volume side quads from straddling the near plane (which would
     * otherwise leave a hole that the open-volume parity counts can't fill
     * in, manifesting as half-shadows or missing shadows for casters near
     * the camera). */
    Vec4 ed4 = view_proj * Vec4(ext_dir_world.x, ext_dir_world.y,
                                ext_dir_world.z, 0.0f);
    const float dB = ed4.z + ed4.w;

    auto extrude_clamped = [&](const Vec3& v_w) -> Vec3 {
        if(dB >= 0.0f) {
            // Extrusion stays in front (or parallel to the near plane).
            return v_w + ext_dir_world * SHADOW_EXTRUSION_DISTANCE;
        }
        Vec4 cv = view_proj * Vec4(v_w.x, v_w.y, v_w.z, 1.0f);
        const float A = cv.z + cv.w;          // clip-space near-plane signed distance
        const float margin = 0.001f;          // keep just in front of the plane
        float t = (margin - A) / dB;          // distance at which we'd hit the plane
        if(t > SHADOW_EXTRUSION_DISTANCE) t = SHADOW_EXTRUSION_DISTANCE;
        if(t < 0.0f) t = 0.0f;                // silhouette itself is behind the plane
        return v_w + ext_dir_world * t;
    };

    for(const auto& edge : sil_edges) {
        // Transform edge vertices from local to world space
        Vec3 v1w = edge.first.transformed_by(world_mat);
        Vec3 v2w = edge.second.transformed_by(world_mat);
        Vec3 v1e = extrude_clamped(v1w);
        Vec3 v2e = extrude_clamped(v2w);

        // Shadow volume side quad (two triangles, CCW winding)
        auto base = (uint32_t)sv_verts_->count();
        sv_verts_->position(v1w); sv_verts_->move_next();
        sv_verts_->position(v2w); sv_verts_->move_next();
        sv_verts_->position(v2e); sv_verts_->move_next();
        sv_verts_->position(v1e); sv_verts_->move_next();

        // Triangle 1: v1w, v2w, v2e
        sv_idx_->index(base);     sv_idx_->index(base + 1); sv_idx_->index(base + 2);
        // Triangle 2: v1w, v2e, v1e
        sv_idx_->index(base);     sv_idx_->index(base + 2); sv_idx_->index(base + 3);
    }
}

void ShadowCaster::do_generate_renderables(batcher::RenderQueue* render_queue,
                                            const Camera* camera,
                                            const Viewport* viewport,
                                            const DetailLevel detail_level,
                                            Light** lights,
                                            const std::size_t light_count) {
    // Rebuild shadow volume geometry for this frame
    sv_verts_->clear();
    sv_idx_->clear();
    sv_volumes_.clear();

    const bool do_shadows = (light_count > 0);
    if(do_shadows) {
        ++cache_generation_;
    }

    /* projection * view — used to clamp the per-vertex extrusion in clip
     * space (see generate_shadow_geometry). Computed once per frame. */
    const Mat4 view_proj = camera->projection_matrix() * camera->view_matrix();

    // Let each descendant generate its renderables into the queue. We track the
    // range of renderables each node produced so that we can build shadow
    // volumes from the actual geometry, regardless of the node's type.
    for(StageNode& node: each_descendent()) {
        if(!node.is_visible() || node.is_destroyed() ||
           node.generates_renderables_for_descendents()) {
            continue;
        }

        const std::size_t start = render_queue->renderable_count();
        node.generate_renderables(render_queue, camera, viewport,
                                  detail_level, lights, light_count);
        const std::size_t end = render_queue->renderable_count();

        if(!light_count || node.shadow_cast() == SHADOW_CAST_NEVER) {
            continue;
        }

        // A single renderable is inserted once per material pass, so skip
        // consecutive duplicates that share geometry and transform.
        const VertexData* last_vd = nullptr;
        const IndexData* last_id = nullptr;
        const Mat4* last_xf = nullptr;

        for(std::size_t idx = start; idx < end; ++idx) {
            const Renderable* r = render_queue->renderable(idx);
            if(!r || !r->vertex_data) continue;

            if(r->vertex_data == last_vd && r->index_data == last_id &&
               r->final_transformation == last_xf) {
                continue;
            }
            last_vd = r->vertex_data;
            last_id = r->index_data;
            last_xf = r->final_transformation;

            // Edge adjacency depends only on the geometry, so fetch it once
            // (from the cache for persistent renderables) and reuse it across
            // every light affecting this renderable.
            const std::vector<EdgeInfo>& edges = adjacency_for(*r);
            if(edges.empty()) {
                continue;
            }

            for(std::size_t i = 0; i < light_count; ++i) {
                Light* light = lights[i];
                if(!light) continue;

                // Only directional shadow volumes are supported for now. Point
                // lights need per-vertex extrusion; spotlights aren't handled.
                if(light->light_type() != LIGHT_TYPE_DIRECTIONAL) {
                    continue;
                }

                // direction() is the vector *towards* the light (w=0 lighting
                // convention). The silhouette is computed in the travel-direction
                // frame, so we must extrude along the travel direction too.
                Vec3 ext_dir = -light->direction();
                if(ext_dir.length_squared() < 1e-6f) continue;
                ext_dir.normalize();

                const uint32_t vol_start = (uint32_t)sv_idx_->count();
                generate_shadow_geometry(*r, edges, light, ext_dir, view_proj);
                const uint32_t vol_end = (uint32_t)sv_idx_->count();
                if(vol_end > vol_start) {
                    sv_volumes_.emplace_back(vol_start, vol_end - vol_start);
                }
            }
        }
    }

    // Evict cached adjacency for persistent renderables that were not seen this
    // frame (the geometry stopped being returned, e.g. node destroyed/hidden).
    if(do_shadows) {
        for(auto it = adjacency_cache_.begin(); it != adjacency_cache_.end();) {
            if(it->second.last_seen != cache_generation_) {
                it = adjacency_cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    if(!sv_idx_->count()) {
        return;
    }

    sv_verts_->done();
    sv_idx_->done();

    const auto sv_count  = sv_idx_->count();
    const Vec3 sv_center = Vec3(); // origin; shadow volumes span the scene

    // Increment-stencil pass (front faces of shadow volume)
    {
        Renderable r;
        r.arrangement           = MESH_ARRANGEMENT_TRIANGLES;
        r.vertex_data           = sv_verts_.get();
        r.index_data            = sv_idx_.get();
        r.index_element_count   = sv_count;
        r.vertex_ranges         = nullptr;
        r.vertex_range_count    = 0;
        r.render_priority       = RENDER_PRIORITY_NEAR;
        r.final_transformation  = &sv_identity_;
        r.material              = sv_mat_incr_.get();
        r.is_visible            = true;
        r.light_count           = 0;
        r.center                = sv_center;
        render_queue->insert_renderable(std::move(r));
    }

    // Decrement-stencil pass (back faces of shadow volume)
    {
        Renderable r;
        r.arrangement           = MESH_ARRANGEMENT_TRIANGLES;
        r.vertex_data           = sv_verts_.get();
        r.index_data            = sv_idx_.get();
        r.index_element_count   = sv_count;
        r.vertex_ranges         = nullptr;
        r.vertex_range_count    = 0;
        r.render_priority       = RENDER_PRIORITY_NEAR;
        r.final_transformation  = &sv_identity_;
        r.material              = sv_mat_decr_.get();
        r.is_visible            = true;
        r.light_count           = 0;
        r.center                = sv_center;
        render_queue->insert_renderable(std::move(r));
    }

    // PVR modifier-volume pass — one renderable per shadow caster × light
    // pair. The PVR closes a modifier volume on each INCLUDE_LAST_POLY and
    // resets per-volume parity, so submitting separate volumes is what makes
    // overlapping shadows from different casters union (rather than XOR
    // themselves out). GL renderers skip these passes at the visitor level
    // because they target POLYGON_LIST_TARGET_MODIFIER.
    for(const auto& vol : sv_volumes_) {
        Renderable r;
        r.arrangement           = MESH_ARRANGEMENT_TRIANGLES;
        r.vertex_data           = sv_verts_.get();
        r.index_data            = sv_idx_.get();
        r.first_index           = vol.first;
        r.index_element_count   = vol.second;
        r.vertex_ranges         = nullptr;
        r.vertex_range_count    = 0;
        r.render_priority       = RENDER_PRIORITY_NEAR;
        r.final_transformation  = &sv_identity_;
        r.material              = sv_mat_modifier_.get();
        r.is_visible            = true;
        r.light_count           = 0;
        r.center                = sv_center;
        render_queue->insert_renderable(std::move(r));
    }

    // Shadow overlay: NDC-space fullscreen triangle, darkens shadowed pixels
    // overlay_transform_ = inv(view) * inv(proj) so that
    //   proj * view * overlay_transform_ * ndc_vertex = ndc_vertex
    overlay_transform_ = camera->transform->world_space_matrix() *
                         camera->projection_matrix().inversed();

    {
        Renderable r;
        r.arrangement           = MESH_ARRANGEMENT_TRIANGLES;
        r.vertex_data           = overlay_verts_.get();
        r.index_data            = overlay_idx_.get();
        r.index_element_count   = overlay_idx_->count();
        r.vertex_ranges         = nullptr;
        r.vertex_range_count    = 0;
        r.render_priority       = RENDER_PRIORITY_FOREGROUND;
        r.final_transformation  = &overlay_transform_;
        r.material              = overlay_mat_.get();
        r.is_visible            = true;
        r.light_count           = 0;
        r.center                = Vec3();
        render_queue->insert_renderable(std::move(r));
    }
}

} // namespace smlt
