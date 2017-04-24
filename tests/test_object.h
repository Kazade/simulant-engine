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

    void test_move_forward_by() {
        auto actor = stage_id_.fetch()->new_actor().fetch();

        actor->rotate_to(smlt::Quaternion(smlt::Degrees(0), smlt::Degrees(90), smlt::Degrees(0)));
        actor->move_forward_by(200);

        assert_close(actor->absolute_position().x, -200.0, 0.0001);
        assert_close(actor->absolute_position().y, 0.0, 0.0001);
        assert_close(actor->absolute_position().z, 0.0, 0.0001);
    }

    void test_positional_constraints() {
        smlt::AABB aabb(Vec3(), 2.0);

        auto actor = stage_id_.fetch()->new_actor().fetch();
        actor->move_to(Vec3(2, 0.5, 0.5));

        assert_equal(2, actor->position().x);

        // Applying the constraint should be enough to change the position
        actor->constrain_to_aabb(aabb);
        assert_equal(1, actor->position().x);

        // Subsequent moves should be constrained
        actor->move_to(Vec3(2, 0.5, 0.5));
        assert_equal(1, actor->position().x);
    }

    void test_scaling_applies() {
        auto actor = window->stage(stage_id_)->new_actor().fetch();
        actor->scale_by(smlt::Vec3(2.0, 1.0, 0.5));

        auto transform = actor->absolute_transformation();

        assert_equal(transform[0], 2.0);
        assert_equal(transform[5], 1.0);
        assert_equal(transform[10], 0.5);

        auto actor2 = window->stage(stage_id_)->new_actor().fetch();

        actor2->rotate_y_by(smlt::Degrees(1.0));

        auto first = actor2->absolute_transformation();

        actor2->scale_by(smlt::Vec3(2.0, 2.0, 2.0));

        auto second = actor2->absolute_transformation();

        assert_equal(first[0] * 2, second[0]);
        assert_equal(first[5] * 2, second[5]);
        assert_equal(first[10] * 2, second[10]);
    }

    void test_set_parent() {
        auto actor1 = window->stage(stage_id_)->new_actor().fetch();
        auto actor2 = window->stage(stage_id_)->new_actor().fetch();

        actor2->set_parent(actor1);

        actor1->move_to(Vec3(0, 10, 0));

        assert_equal(Vec3(0, 10, 0), actor2->absolute_position());
        assert_equal(Vec3(), actor2->position());

        actor1->rotate_to(Quaternion(Degrees(0), Degrees(90), Degrees(0)));

        assert_close(actor2->absolute_rotation().x, actor1->rotation().x, 0.00001);
        assert_close(actor2->absolute_rotation().y, actor1->rotation().y, 0.00001);
    }

    void test_set_absolute_rotation() {
        smlt::ActorID act = window->stage(stage_id_)->new_actor();
        auto actor = window->stage(stage_id_)->actor(act);

        actor->rotate_to_absolute(smlt::Degrees(10), 0, 0, 1);

        assert_equal(actor->rotation(), actor->absolute_rotation());

        smlt::ActorID act2 = window->stage(stage_id_)->new_actor();
        auto actor2 = window->stage(stage_id_)->actor(act2);

        actor2->set_parent(act);

        assert_equal(actor2->absolute_rotation(), actor->absolute_rotation());

        actor2->rotate_to_absolute(smlt::Degrees(20), 0, 0, 1);

        smlt::Quaternion expected_rel(smlt::Vec3::POSITIVE_Z, smlt::Degrees(10));
        smlt::Quaternion expected_abs(smlt::Vec3::POSITIVE_Z, smlt::Degrees(20));

        assert_close(expected_abs.x, actor2->absolute_rotation().x, 0.000001);
        assert_close(expected_abs.y, actor2->absolute_rotation().y, 0.000001);
        assert_close(expected_abs.z, actor2->absolute_rotation().z, 0.000001);
        assert_close(expected_abs.w, actor2->absolute_rotation().w, 0.000001);

        assert_close(expected_rel.x, actor2->rotation().x, 0.000001);
        assert_close(expected_rel.y, actor2->rotation().y, 0.000001);
        assert_close(expected_rel.z, actor2->rotation().z, 0.000001);
        assert_close(expected_rel.w, actor2->rotation().w, 0.000001);
    }

    void test_set_absolute_position() {
        smlt::ActorID act = window->stage(stage_id_)->new_actor();
        auto actor = window->stage(stage_id_)->actor(act);

        actor->move_to_absolute(10, 10, 10);

        assert_equal(smlt::Vec3(10, 10, 10), actor->absolute_position());

        smlt::ActorID act2 = window->stage(stage_id_)->new_actor();
        auto actor2 = window->stage(stage_id_)->actor(act2);

        actor2->set_parent(act);

        //Should be the same as its parent
        assert_equal(actor2->absolute_position(), actor->absolute_position());

        //Make sure relative position is correctly calculated
        actor2->move_to_absolute(20, 10, 10);
        assert_equal(smlt::Vec3(10, 0, 0), actor2->position());
        assert_equal(smlt::Vec3(20, 10, 10), actor2->absolute_position());

        //Make sure setting by vec3 works
        actor2->move_to_absolute(smlt::Vec3(0, 0, 0));
        assert_equal(smlt::Vec3(), actor2->absolute_position());
    }

    void test_set_relative_position() {
        smlt::ActorID act = window->stage(stage_id_)->new_actor();
        auto actor = window->stage(stage_id_)->actor(act);

        actor->move_to(10, 10, 10);

        //No parent, both should be the same
        assert_equal(smlt::Vec3(10, 10, 10), actor->position());
        assert_equal(smlt::Vec3(10, 10, 10), actor->absolute_position());

        smlt::ActorID act2 = window->stage(stage_id_)->new_actor();
        auto actor2 = window->stage(stage_id_)->actor(act2);

        actor2->set_parent(act);

        actor2->move_to(smlt::Vec3(10, 0, 0));

        assert_equal(smlt::Vec3(20, 10, 10), actor2->absolute_position());
        assert_equal(smlt::Vec3(10, 0, 0), actor2->position());
    }

private:
    smlt::CameraID camera_id_;
    smlt::StageID stage_id_;
};

#endif // TEST_OBJECT_H
