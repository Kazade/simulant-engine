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
        mesh_ = stage_->assets->new_mesh_as_cube(1.0f);
        camera_ = stage_->new_camera();

        stage_->assets->set_garbage_collection_grace_period(0);
    }

    void test_shared_vertex_vbo() {
        auto ret1 = vbo_manager_->allocate_slot(mesh_->vertex_data);
        VBO* vbo = ret1.first;
        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 1);

        auto mesh2 = stage_->assets->new_mesh_as_cube(1.0f);

        auto ret3 = vbo_manager_->allocate_slot(mesh2->vertex_data);
        assert_equal(ret1.first, ret3.first);
        assert_not_equal(ret1.second, ret3.second); // New slot, same VBO

        assert_equal(vbo->used_slot_count(), 2u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 2);

        stage_->assets->delete_mesh(mesh2->id());
        mesh2.reset(); // Remove refcount
        stage_->assets->run_garbage_collection();

        // Slot should've been freed
        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 1);
    }

    void test_shared_index_vbo() {
        auto ret1 = vbo_manager_->allocate_slot(mesh_->first_submesh()->index_data);
        VBO* vbo = ret1.first;
        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 1);

        auto mesh2 = stage_->assets->new_mesh_as_cube(1.0f);

        auto ret3 = vbo_manager_->allocate_slot(mesh2->first_submesh()->index_data);
        assert_equal(ret1.first, ret3.first);
        assert_not_equal(ret1.second, ret3.second); // New slot, same VBO

        assert_equal(vbo->used_slot_count(), 2u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 2);

        stage_->assets->delete_mesh(mesh2->id());
        mesh2.reset(); // Remove refcount
        stage_->assets->run_garbage_collection();

        // Slot should've been freed
        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 1);
    }

    void test_dedicated_vbo() {
        auto ret1 = vbo_manager_->allocate_slot(mesh_->vertex_data);
        VBO* vbo = ret1.first;
        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 1);

        auto mesh2 = stage_->assets->new_mesh(VertexSpecification::DEFAULT);

        /* 50000 verts should tip over 512k always */
        for(auto i = 0; i < 50000; ++i) {
            mesh2->vertex_data->position(Vec3());
            mesh2->vertex_data->move_next();
        }
        mesh2->vertex_data->done();

        auto ret2 = vbo_manager_->allocate_slot(mesh2->vertex_data);
        assert_equal(vbo_manager_->dedicated_buffer_count(), 1u);

        vbo = ret2.first;
        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo->free_slot_count(), 0u); // No free slots in a dedicated one

        stage_->assets->delete_mesh(mesh2->id());
        mesh2.reset(); // Remove refcount
        stage_->assets->run_garbage_collection();

        assert_equal(vbo_manager_->dedicated_buffer_count(), 0u);
    }

    void test_promotion_to_dedicated() {
        auto ret1 = vbo_manager_->allocate_slot(mesh_->vertex_data);
        VBO* vbo = ret1.first;
        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo->free_slot_count(), (VBO_SIZE / vbo->slot_size_in_bytes()) - 1);

        /* 50000 verts should tip over 512k always */
        for(auto i = 0; i < 50000; ++i) {
            mesh_->vertex_data->position(Vec3());
            mesh_->vertex_data->move_next();
        }
        mesh_->vertex_data->done();

        assert_equal(vbo->used_slot_count(), 1u);
        assert_equal(vbo_manager_->dedicated_buffer_count(), 0u);

        auto actor = stage_->new_actor_with_mesh(mesh_->id());

        vbo_manager_->update_and_fetch_buffers(actor->_get_renderables(camera_, DETAIL_LEVEL_NEAREST)[0].get());

        assert_equal(vbo_manager_->dedicated_buffer_count(), 1u);

        // VBO is not destroyed, maybe it should be?
        assert_equal(vbo->used_slot_count(), 0u);
    }

    void test_demotion_to_shared() {
        throw test::SkippedTestError("Demotion from dedicated to shared VBOs when data reduces in size is not yet implemented. See #192");
    }
};

}
