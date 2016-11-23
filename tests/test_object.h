#ifndef TEST_OBJECT_H
#define TEST_OBJECT_H

#include "simulant/simulant.h"
#include "kaztest/kaztest.h"

#include "global.h"

class ObjectTest : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        camera_id_ = window->new_camera();
        stage_id_ = window->new_stage();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        window->delete_camera(camera_id_);
        window->delete_stage(stage_id_);
    }

    void test_scaling_applies() {
        auto actor = window->stage(stage_id_)->new_actor().fetch();
        actor->set_relative_scale(smlt::Vec3(2.0, 1.0, 0.5));

        auto transform = actor->absolute_transformation();

        assert_equal(transform[0], 2.0);
        assert_equal(transform[5], 1.0);
        assert_equal(transform[10], 0.5);

        auto actor2 = window->stage(stage_id_)->new_actor().fetch();

        actor2->rotate_y(smlt::Degrees(1.0));

        auto first = actor2->absolute_transformation();

        actor2->set_relative_scale(smlt::Vec3(2.0, 2.0, 2.0));

        auto second = actor2->absolute_transformation();

        assert_equal(first[0] * 2, second[0]);
        assert_equal(first[5] * 2, second[5]);
        assert_equal(first[10] * 2, second[10]);
    }

    void test_set_absolute_rotation() {
        smlt::ActorID act = window->stage(stage_id_)->new_actor();
        auto actor = window->stage(stage_id_)->actor(act);

        actor->set_absolute_rotation(smlt::Degrees(10), 0, 0, 1);

        assert_equal(actor->relative_rotation(), actor->absolute_rotation());

        smlt::ActorID act2 = window->stage(stage_id_)->new_actor();
        auto actor2 = window->stage(stage_id_)->actor(act2);

        actor2->set_parent(act);

        assert_equal(actor2->absolute_rotation(), actor->absolute_rotation());

        actor2->set_absolute_rotation(smlt::Degrees(20), 0, 0, 1);

        smlt::Quaternion expected_rel, expected_abs;
        kmQuaternionRotationAxisAngle(&expected_abs, &KM_VEC3_POS_Z, kmDegreesToRadians(20));
        kmQuaternionRotationAxisAngle(&expected_rel, &KM_VEC3_POS_Z, kmDegreesToRadians(10));

        assert_equal(expected_abs, actor2->absolute_rotation());
        assert_equal(expected_rel, actor2->relative_rotation());
    }

    void test_set_absolute_position() {
        smlt::ActorID act = window->stage(stage_id_)->new_actor();
        auto actor = window->stage(stage_id_)->actor(act);

        actor->set_absolute_position(10, 10, 10);

        assert_equal(smlt::Vec3(10, 10, 10), actor->absolute_position());

        smlt::ActorID act2 = window->stage(stage_id_)->new_actor();
        auto actor2 = window->stage(stage_id_)->actor(act2);

        actor2->set_parent(act);

        //Should be the same as its parent
        assert_equal(actor2->absolute_position(), actor->absolute_position());

        //Make sure relative position is correctly calculated
        actor2->set_absolute_position(20, 10, 10);
        assert_equal(smlt::Vec3(10, 0, 0), actor2->relative_position());
        assert_equal(smlt::Vec3(20, 10, 10), actor2->absolute_position());

        //Make sure setting by vec3 works
        actor2->set_absolute_position(smlt::Vec3(0, 0, 0));
        assert_equal(smlt::Vec3(), actor2->absolute_position());
    }

    void test_set_relative_position() {
        smlt::ActorID act = window->stage(stage_id_)->new_actor();
        auto actor = window->stage(stage_id_)->actor(act);

        actor->set_relative_position(10, 10, 10);

        //No parent, both should be the same
        assert_equal(smlt::Vec3(10, 10, 10), actor->relative_position());
        assert_equal(smlt::Vec3(10, 10, 10), actor->absolute_position());

        smlt::ActorID act2 = window->stage(stage_id_)->new_actor();
        auto actor2 = window->stage(stage_id_)->actor(act2);

        actor2->set_parent(act);

        actor2->set_relative_position(smlt::Vec3(10, 0, 0));

        assert_equal(smlt::Vec3(20, 10, 10), actor2->absolute_position());
        assert_equal(smlt::Vec3(10, 0, 0), actor2->relative_position());
    }

private:
    smlt::CameraID camera_id_;
    smlt::StageID stage_id_;
};

#endif // TEST_OBJECT_H
