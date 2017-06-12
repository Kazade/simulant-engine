#pragma once

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"

namespace {

using namespace smlt;

class RenderQueueTests : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = window->stage(window->new_stage());
    }

    void tear_down() {
        window->delete_stage(stage_->id());
    }

    void test_material_change_updates_queue() {
        auto& render_queue = stage_->render_queue;

        auto texture_1 = stage_->assets->new_texture();
        auto texture_2 = stage_->assets->new_texture();

        stage_->assets->texture(texture_1)->upload();
        stage_->assets->texture(texture_2)->upload();

        auto mat_1 = stage_->assets->new_material_from_texture(texture_1);
#ifndef SIMULANT_GL_VERSION_1X
        mat_1.fetch()->delete_pass(1); // Delete the lighting pass from the material
#endif

        auto mat_2 = stage_->assets->new_material_from_texture(texture_2);
#ifndef SIMULANT_GL_VERSION_1X
        mat_2.fetch()->delete_pass(1);
#endif
        auto mesh_1 = stage_->assets->new_mesh_as_cube(1.0);
        stage_->assets->mesh(mesh_1)->set_material_id(mat_1);

        auto actor_id = stage_->new_actor_with_mesh(mesh_1);

        assert_equal(1, render_queue->pass_count());

        // Store the current group. We don't give direct access to batches and groups aside
        // from iteration so that's why the code is a bit funky. Also, RenderGroup doesn't have a
        // default constructor - hence the shared_ptr and copy construction.
        typedef smlt::batcher::RenderGroup RenderGroup;
        std::shared_ptr<RenderGroup> group;
        render_queue->each_group(0, [&](uint32_t i, const RenderGroup& grp, const smlt::batcher::Batch&) {
             group.reset(new RenderGroup(grp));
        });

        // Setting a new material on the mesh should update the actor and subactors
        // in the render queue, and because the texture ID is higher, then this should make
        // the new group > the old one.

        stage_->actor(actor_id)->override_material_id(mat_2);

        assert_equal(1, render_queue->group_count(0));

        render_queue->each_group(0, [&](uint32_t i, const RenderGroup& grp, const smlt::batcher::Batch&) {
             assert_true(*group < grp);
        });

        stage_->actor(actor_id)->remove_material_id_override();
        stage_->assets->mesh(mesh_1)->set_material_id(mat_1);

        assert_equal(1, render_queue->group_count(0));

        // Everything should be back to the first material now
        render_queue->each_group(0, [&](uint32_t i, const RenderGroup& grp, const smlt::batcher::Batch&) {
             assert_true(!(*group < grp));
        });

        stage_->assets->mesh(mesh_1)->set_material_id(mat_2);

        assert_equal(1, render_queue->group_count(0));

        // Back to the second material again!
        render_queue->each_group(0, [&](uint32_t i, const RenderGroup& grp, const smlt::batcher::Batch&) {
             assert_true(*group < grp);
        });
    }

    void test_renderable_removal() {
        auto& render_queue = stage_->render_queue;

        auto mesh_1 = stage_->assets->new_mesh_as_cube(1.0);
        auto actor_id = stage_->new_actor_with_mesh(mesh_1);

#ifdef SIMULANT_GL_VERSION_1X
        assert_equal(1, render_queue->pass_count());
        assert_equal(1, render_queue->group_count(0));
#else
        assert_equal(2, render_queue->pass_count());
        assert_equal(1, render_queue->group_count(0));
        assert_equal(1, render_queue->group_count(1));
#endif
        stage_->delete_actor(actor_id);

        assert_equal(0, render_queue->pass_count());
    }

    void test_texture_grouping() {
        auto texture_1 = stage_->assets->new_texture();
        auto texture_2 = stage_->assets->new_texture();

        stage_->assets->texture(texture_1)->upload();
        stage_->assets->texture(texture_2)->upload();

        auto mat_1 = stage_->assets->new_material_from_texture(texture_1);
#ifndef SIMULANT_GL_VERSION_1X
        mat_1.fetch()->delete_pass(1);
#endif
        auto mat_2 = stage_->assets->new_material_from_texture(texture_2);
#ifndef SIMULANT_GL_VERSION_1X
        mat_2.fetch()->delete_pass(1);
#endif
        auto mat_3 = stage_->assets->new_material_from_texture(texture_1); // Texture 1 repeated
#ifndef SIMULANT_GL_VERSION_1X
        mat_3.fetch()->delete_pass(1);
#endif
        auto& render_queue = stage_->render_queue;

        assert_equal(0, render_queue->pass_count());

        auto mesh_1 = stage_->assets->new_mesh_as_cube(1.0);
        stage_->assets->mesh(mesh_1)->set_material_id(mat_1);

        auto mesh_2 = stage_->assets->new_mesh_as_cube(1.0);
        stage_->assets->mesh(mesh_2)->set_material_id(mat_2);

        auto mesh_3 = stage_->assets->new_mesh_as_cube(1.0);
        stage_->assets->mesh(mesh_3)->set_material_id(mat_3);

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

#ifdef SIMULANT_GL_VERSION_2X
    void test_shader_grouping() {

    }
#else
    void test_shader_grouping() {}
#endif

private:
    StagePtr stage_;

};

}
