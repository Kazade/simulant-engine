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

    void test_texture_grouping() {
        auto texture_1 = stage_->new_texture();
        auto texture_2 = stage_->new_texture();

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
