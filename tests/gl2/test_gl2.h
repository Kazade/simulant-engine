#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "../../simulant/renderers/gl2x/vbo_manager.h"

namespace {

using namespace smlt;

class VBOManagerTests:
    public smlt::test::SimulantTestCase {

private:
    VBOManager::ptr vbo_manager_;
    StagePtr stage_;
    MeshPtr mesh_;
    CameraPtr camera_;
public:
    void set_up() {
        smlt::test::SimulantTestCase::set_up();

        vbo_manager_ = VBOManager::create();
        stage_ = window->new_stage();
        mesh_ = stage_->assets->new_mesh_as_cube(1.0f).fetch();
        camera_ = stage_->new_camera();
    }

    void test_shared_vbo() {
        auto ret1 = vbo_manager_->allocate_slot(mesh_->vertex_data);
        VBO* vbo = ret1.first;
        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 1);

        auto mesh2 = stage_->assets->new_mesh_as_cube(1.0f).fetch();

        auto ret3 = vbo_manager_->allocate_slot(mesh2->vertex_data);
        assert_equal(ret1.first, ret3.first);
        assert_not_equal(ret1.second, ret3.second); // New slot, same VBO

        assert_equal(vbo->used_slot_count(), 2u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 2);
    }

    void test_dedicated_vbo() {

    }

    void test_promotion_to_dedicated() {

    }

    void test_multiple_specifications() {

    }

    void test_demotion_to_shared() {

    }

    void test_vertex_data_destruction() {

    }

    void test_index_data_destruction() {

    }
};

}
