#ifndef TEST_OBJECT_H
#define TEST_OBJECT_H

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class ObjectTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage_ = window->new_stage();
        camera_ = stage_->new_camera();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        window->destroy_stage(stage_->id());
    }

    void test_move_to_origin() {
        auto pipeline = window->render(stage_, camera_);

        // A bug was reported that this caused a crash (see #219)
        auto mesh = stage_->assets->new_mesh_as_cube(50.0f);
        auto actor = stage_->new_actor_with_mesh(mesh);
        actor->move_to(0, 0, 0);
        window->run_frame();
    }

    void test_move_forward_by() {
        auto actor = stage_->new_actor();

        actor->rotate_to(smlt::Quaternion(smlt::Degrees(0), smlt::Degrees(90), smlt::Degrees(0)));
        actor->move_forward_by(200);

        assert_close(actor->absolute_position().x, -200.0, 0.0001);
        assert_close(actor->absolute_position().y, 0.0, 0.0001);
        assert_close(actor->absolute_position().z, 0.0, 0.0001);
    }

    void test_positional_constraints() {
        smlt::AABB aabb(Vec3(), 2.0);

        auto actor = stage_->new_actor();
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
        auto actor = stage_->new_actor();
        actor->scale_by(smlt::Vec3(2.0, 1.0, 0.5));

        auto transform = actor->absolute_transformation();

        assert_equal(transform[0], 2.0);
        assert_equal(transform[5], 1.0);
        assert_equal(transform[10], 0.5);

        auto actor2 = stage_->new_actor();

        actor2->rotate_y_by(smlt::Degrees(1.0));

        auto first = actor2->absolute_transformation();

        actor2->scale_by(smlt::Vec3(2.0, 2.0, 2.0));

        auto second = actor2->absolute_transformation();

        assert_equal(first[0] * 2, second[0]);
        assert_equal(first[5] * 2, second[5]);
        assert_equal(first[10] * 2, second[10]);
    }

    void test_set_parent() {
        auto actor1 = stage_->new_actor();
        auto actor2 = stage_->new_actor();

        actor2->set_parent(actor1);

        actor1->move_to(Vec3(0, 10, 0));

        assert_equal(Vec3(0, 10, 0), actor2->absolute_position());
        assert_equal(Vec3(), actor2->position());

        actor1->rotate_to(Quaternion(Degrees(0), Degrees(90), Degrees(0)));

        assert_close(actor2->absolute_rotation().x, actor1->rotation().x, 0.00001);
        assert_close(actor2->absolute_rotation().y, actor1->rotation().y, 0.00001);
    }

    void test_parent_transformation_applied() {
        auto actor1 = stage_->new_actor();
        auto actor2 = stage_->new_actor();
        actor2->set_parent(actor1);

        actor2->move_to(smlt::Vec3(0, 0, -5));

        actor1->move_to(smlt::Vec3(0, 0, -5));
        actor1->rotate_to(smlt::Quaternion(smlt::Degrees(0), smlt::Degrees(90), smlt::Degrees(0)));

        assert_close(actor2->absolute_position().x, -5.0f, 0.00001f);
        assert_close(actor2->absolute_position().y,  0.0f, 0.00001f);
        assert_close(actor2->absolute_position().z, -5.0f, 0.00001f);
    }

    void test_set_absolute_rotation() {
        auto actor = stage_->new_actor();

        actor->rotate_to_absolute(smlt::Degrees(10), 0, 0, 1);

        assert_equal(actor->rotation(), actor->absolute_rotation());

        auto actor2 = stage_->new_actor();

        actor2->set_parent(actor->id());

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
        auto actor = stage_->new_actor();

        actor->move_to_absolute(10, 10, 10);

        assert_equal(smlt::Vec3(10, 10, 10), actor->absolute_position());

        auto actor2 = stage_->new_actor();

        actor2->set_parent(actor->id());

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
        auto actor = stage_->new_actor();

        actor->move_to(10, 10, 10);

        //No parent, both should be the same
        assert_equal(smlt::Vec3(10, 10, 10), actor->position());
        assert_equal(smlt::Vec3(10, 10, 10), actor->absolute_position());

        auto actor2 = stage_->new_actor();

        actor2->set_parent(actor->id());

        actor2->move_to(smlt::Vec3(10, 0, 0));

        assert_equal(smlt::Vec3(20, 10, 10), actor2->absolute_position());
        assert_equal(smlt::Vec3(10, 0, 0), actor2->position());
    }

    void test_move_updates_children() {
        auto actor1 = stage_->new_actor();
        auto actor2 = stage_->new_actor();

        actor2->move_to(0, 0, 10.0f);
        actor2->set_parent(actor1);

        assert_equal(10.0f, actor2->absolute_position().z);
    }

    void test_set_parent_to_self_does_nothing() {
        auto actor1 = stage_->new_actor();

        auto original_parent = actor1->parent();
        actor1->set_parent(actor1);
        assert_equal(original_parent, actor1->parent());
    }

    void test_find_child_by_name() {
        auto actor1 = stage_->new_actor_with_name("actor1");

        assert_equal(actor1, stage_->find_child_with_name("actor1"));
        assert_is_null(stage_->find_child_with_name("bananas"));

        auto dupe = stage_->new_actor_with_name("actor1");
        dupe->set_parent(actor1);

        // Just find the first one
        assert_equal(actor1, stage_->find_child_with_name("actor1"));
    }

private:
    smlt::CameraPtr camera_;
    smlt::StagePtr stage_;
};

}
#endif // TEST_OBJECT_H
