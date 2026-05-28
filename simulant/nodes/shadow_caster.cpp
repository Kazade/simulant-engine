#include "shadow_caster.h"

#include "actor.h"
#include "light.h"
#include "camera.h"
#include "../scenes/scene.h"
#include "../asset_manager.h"
#include "../renderers/batching/render_queue.h"
#include "../shadows.h"
#include "../meshes/mesh.h"

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
    // Vertices in NDC: combined with overlay_transform_ they cover the whole screen
    overlay_verts_ = std::make_unique<VertexData>(VertexSpecification::POSITION_ONLY);
    overlay_idx_   = std::make_unique<IndexData>(INDEX_TYPE_8_BIT);

    overlay_verts_->position(Vec3(-1.0f, -1.0f, 0.0f)); overlay_verts_->move_next();
    overlay_verts_->position(Vec3( 3.0f, -1.0f, 0.0f)); overlay_verts_->move_next();
    overlay_verts_->position(Vec3(-1.0f,  3.0f, 0.0f)); overlay_verts_->move_next();
    overlay_verts_->done();

    overlay_idx_->index(0); overlay_idx_->index(1); overlay_idx_->index(2);
    overlay_idx_->done();

    // sv_identity_ is already identity-initialised by Mat4 default constructor

    return true;
}

void ShadowCaster::generate_shadow_geometry(const MeshPtr& mesh,
                                             const Mat4& world_mat,
                                             LightPtr light,
                                             const Vec3& ext_dir_world) {
    MeshSilhouette silhouette(mesh, world_mat, light);
    const auto& edges = silhouette.edge_list();
    if(edges.empty()) {
        return;
    }

    for(const auto& edge : edges) {
        // Transform edge vertices from mesh-local to world space
        Vec3 v1w = edge.first.transformed_by(world_mat);
        Vec3 v2w = edge.second.transformed_by(world_mat);
        Vec3 v1e = v1w + ext_dir_world * SHADOW_EXTRUSION_DISTANCE;
        Vec3 v2e = v2w + ext_dir_world * SHADOW_EXTRUSION_DISTANCE;

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
    // First, let all descendants generate their own renderables normally
    for(StageNode& node: each_descendent()) {
        if(node.is_visible() && !node.is_destroyed() &&
           !node.generates_renderables_for_descendents()) {
            node.generate_renderables(render_queue, camera, viewport,
                                      detail_level, lights, light_count);
        }
    }

    if(!light_count) {
        return;
    }

    // Rebuild shadow volume geometry for this frame
    sv_verts_->clear();
    sv_idx_->clear();

    // Find all shadow-casting Actor descendants
    auto actors = find_descendents_by_types({Actor::Meta::node_type});

    for(auto* node : actors) {
        auto* actor = static_cast<Actor*>(node);
        if(!actor->is_visible() || actor->is_destroyed()) continue;
        if(actor->shadow_cast() == SHADOW_CAST_NEVER) continue;
        if(!actor->has_any_mesh()) continue;

        const MeshPtr& mesh = actor->base_mesh();
        if(!mesh) continue;

        const Mat4& world_mat = actor->transform->world_space_matrix();

        for(std::size_t i = 0; i < light_count; ++i) {
            Light* light = lights[i];
            if(!light) continue;

            Vec3 ext_dir;
            if(light->light_type() == LIGHT_TYPE_DIRECTIONAL) {
                // direction() is the vector *towards* the light (w=0 lighting
                // convention). The silhouette is computed in the travel-direction
                // frame, so we must extrude along travel direction too.
                ext_dir = -light->direction();
                if(ext_dir.length_squared() < 1e-6f) continue;
                ext_dir.normalize();
            } else if(light->light_type() == LIGHT_TYPE_POINT) {
                // For point lights, extrusion direction is per-vertex; we use the
                // mesh centre as an approximation for the silhouette calculation
                // (MeshSilhouette handles the actual per-edge direction internally)
                // The extrusion direction stored here is unused for point lights;
                // we generate separate geometry per silhouette edge below.
                // TODO: per-edge extrusion for point light shadow volumes
                ext_dir = Vec3(); // not used
            } else {
                continue; // Spotlights not yet supported
            }

            if(light->light_type() == LIGHT_TYPE_DIRECTIONAL) {
                generate_shadow_geometry(mesh, world_mat, light, ext_dir);
            }
            // Point light shadow volumes require per-vertex extrusion and
            // are not yet implemented.
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
