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
