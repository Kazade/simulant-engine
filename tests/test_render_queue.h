#pragma once

#include <kaztest/kaztest.h>

#include "kglt/kglt.h"

namespace {

using namespace kglt;

class RenderQueueTests : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();
        stage_ = window->stage(window->new_stage());
    }

    void tear_down() {
        window->delete_stage(stage_->id());
    }

    void test_material_change_updates_queue() {
        auto& render_queue = stage_->render_queue;

        auto texture_1 = stage_->new_texture();
        auto texture_2 = stage_->new_texture();

        stage_->texture(texture_1)->upload();
        stage_->texture(texture_2)->upload();

        auto mat_1 = stage_->new_material_from_texture(texture_1);
        auto mat_2 = stage_->new_material_from_texture(texture_2);

        auto mesh_1 = stage_->new_mesh_as_cube(1.0);
        stage_->mesh(mesh_1)->set_material_id(mat_1);

        auto actor_id = stage_->new_actor_with_mesh(mesh_1);

        assert_equal(1, render_queue->pass_count());

        // Store the current group. We don't give direct access to batches and groups aside
        // from iteration so that's why the code is a bit funky. Also, RenderGroup doesn't have a
        // default constructor - hence the shared_ptr and copy construction.
        typedef kglt::new_batcher::RenderGroup RenderGroup;
        std::shared_ptr<RenderGroup> group;
        render_queue->each_group(0, [&](uint32_t i, const RenderGroup& grp, const kglt::new_batcher::Batch&) {
             group.reset(new RenderGroup(grp));
        });

        // Setting a new material on the mesh should update the actor and subactors
        // in the render queue, and because the texture ID is higher, then this should make
        // the new group > the old one.

        stage_->actor(actor_id)->override_material_id(mat_2);

        assert_equal(1, render_queue->group_count(0));

        render_queue->each_group(0, [&](uint32_t i, const RenderGroup& grp, const kglt::new_batcher::Batch&) {
             assert_true(*group < grp);
        });

        stage_->actor(actor_id)->remove_material_id_override();
        stage_->mesh(mesh_1)->set_material_id(mat_1);

        assert_equal(1, render_queue->group_count(0));

        // Everything should be back to the first material now
        render_queue->each_group(0, [&](uint32_t i, const RenderGroup& grp, const kglt::new_batcher::Batch&) {
             assert_true(!(*group < grp));
        });

        stage_->mesh(mesh_1)->set_material_id(mat_2);

        assert_equal(1, render_queue->group_count(0));

        // Back to the second material again!
        render_queue->each_group(0, [&](uint32_t i, const RenderGroup& grp, const kglt::new_batcher::Batch&) {
             assert_true(*group < grp);
        });
    }

    void test_renderable_removal() {
        auto& render_queue = stage_->render_queue;

        auto mesh_1 = stage_->new_mesh_as_cube(1.0);
        auto actor_id = stage_->new_actor_with_mesh(mesh_1);

        assert_equal(2, render_queue->pass_count());
        assert_equal(1, render_queue->group_count(0));
        assert_equal(1, render_queue->group_count(1));

        stage_->delete_actor(actor_id);

        assert_equal(0, render_queue->pass_count());
    }

    void test_texture_grouping() {
        auto texture_1 = stage_->new_texture();
        auto texture_2 = stage_->new_texture();

        stage_->texture(texture_1)->upload();
        stage_->texture(texture_2)->upload();

        auto mat_1 = stage_->new_material_from_texture(texture_1);
        auto mat_2 = stage_->new_material_from_texture(texture_2);
        auto mat_3 = stage_->new_material_from_texture(texture_1); // Texture 1 repeated

        auto& render_queue = stage_->render_queue;

        assert_equal(0, render_queue->pass_count());

        auto mesh_1 = stage_->new_mesh_as_cube(1.0);
        stage_->mesh(mesh_1)->set_material_id(mat_1);

        auto mesh_2 = stage_->new_mesh_as_cube(1.0);
        stage_->mesh(mesh_2)->set_material_id(mat_2);

        auto mesh_3 = stage_->new_mesh_as_cube(1.0);
        stage_->mesh(mesh_3)->set_material_id(mat_3);

        stage_->new_actor_with_mesh(mesh_1);

        assert_equal(1, render_queue->pass_count());
        assert_equal(1, render_queue->group_count(0));

        stage_->new_actor_with_mesh(mesh_1);

        // Still only 1 pass, with 1 render group (although that
        // should have 2 renderables)
        assert_equal(1, render_queue->pass_count());
        assert_equal(1, render_queue->group_count(0));

        stage_->new_actor_with_mesh(mesh_2);

        assert_equal(1, render_queue->pass_count());
        assert_equal(2, render_queue->group_count(0));

        stage_->new_actor_with_mesh(mesh_3);

        assert_equal(1, render_queue->pass_count());
        assert_equal(2, render_queue->group_count(0));
    }

#ifdef KGLT_GL_VERSION_2X
    void test_shader_grouping() {

    }
#endif

private:
    StagePtr stage_;

};

}
