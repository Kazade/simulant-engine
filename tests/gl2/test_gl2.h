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

    void test_find_buffer() {
        ActorPtr actor = stage_->new_actor_with_mesh(mesh_->id());

        auto renderable = actor->_get_renderables(
            camera_->frustum(),
            DETAIL_LEVEL_NEAREST
        )[0];

        auto buffer = vbo_manager_->find_buffer(renderable.get());
        assert_is_null(buffer);

        auto allocated = vbo_manager_->allocate_buffer(renderable.get());
        assert_equal(allocated, vbo_manager_->find_buffer(renderable.get()));
    }

    void test_allocate_buffer() {
        ActorPtr actor = stage_->new_actor_with_mesh(mesh_->id());

        auto renderable = actor->_get_renderables(
            camera_->frustum(),
            DETAIL_LEVEL_NEAREST
        )[0];

        GPUBuffer* allocated = vbo_manager_->allocate_buffer(renderable.get());

        assert_true(allocated->vertex_vbo);
        assert_equal(allocated->vertex_vbo_slot, 0u);

        assert_true(allocated->index_vbo);
        assert_equal(allocated->index_vbo_slot, 0u);

        /* Should return the same buffer if we try to allocate again */
        assert_equal(allocated, vbo_manager_->allocate_buffer(renderable.get()));
    }
};

class VBOTests:
    public smlt::test::SimulantTestCase {

private:
    VBO* vbo_ = nullptr;

public:
    void test_allocate_slot() {

    }

    void test_release_slot() {

    }

    void test_upload() {

    }

    void test_bind() {

    }
};

}
