#pragma once

#include "simulant/test.h"

namespace {

using namespace smlt;

class RigTests : public test::SimulantTestCase {
public:
    void set_up() {
        test::SimulantTestCase::set_up();
        stage_ = window->new_stage();
    }

    void tear_down() {
        stage_->destroy();
        test::SimulantTestCase::tear_down();
    }

    void test_actor_is_rigged() {
        auto m = stage_->assets->new_mesh(VertexSpecification::DEFAULT);
        auto a1 = stage_->new_actor_with_mesh(m);

        assert_false(a1->is_rigged());

        m->add_skeleton(1);

        assert_true(m->has_skeleton());
        assert_true(a1->is_rigged());

        auto m2 = stage_->assets->new_mesh(VertexSpecification::DEFAULT);

        a1->set_mesh(m2);

        assert_false(m2->has_skeleton());
        assert_false(a1->is_rigged());
    }

    void test_rig_joint_count() {
        auto m = stage_->assets->new_mesh(VertexSpecification::DEFAULT);
        m->add_skeleton(5);

        auto a1 = stage_->new_actor_with_mesh(m);

        assert_true(a1->is_rigged());

        assert_equal(a1->rig->joint_count(), 5u);
    }

private:
    StagePtr stage_;
};

}
