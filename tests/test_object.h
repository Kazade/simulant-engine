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

        stage_ = new_stage();
        camera_ = stage_->new_camera();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        destroy_stage(stage_->id());
    }

    void test_move_to_origin() {
        window->compositor->render(stage_, camera_);

        // A bug was reported that this caused a crash (see #219)
        auto mesh = stage_->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_cube("cube", stage_->assets->new_material(), 50.0f);

        auto actor = stage_->new_actor_with_mesh(mesh);
        actor->move_to(0, 0, 0);
        application->run_frame();
    }

    void test_move_forward_by() {
        auto actor = stage_->new_actor();

        actor->rotate_to(smlt::Quaternion(smlt::Degrees(0), smlt::Degrees(90), smlt::Degrees(0)));
        actor->move_forward_by(200);

        assert_close(actor->absolute_position().x, -200.0f, 0.0001f);
        assert_close(actor->absolute_position().y, 0.0f, 0.0001f);
        assert_close(actor->absolute_position().z, 0.0f, 0.0001f);
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

        assert_equal(transform[0], 2.0f);
        assert_equal(transform[5], 1.0f);
        assert_equal(transform[10], 0.5f);

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

        assert_close(actor2->absolute_rotation().x, actor1->rotation().x, 0.00001f);
        assert_close(actor2->absolute_rotation().y, actor1->rotation().y, 0.00001f);
    }

    void test_parent_is_stage() {
        auto a1 = stage_->new_actor();
        auto a2 = stage_->new_actor();
        auto a3 = stage_->new_actor();

        a2->set_parent(a1);
        a3->set_parent(a2);

        assert_true(a1->parent_is_stage());
        assert_false(a2->parent_is_stage());
        assert_false(a3->parent_is_stage());

        a3->set_parent(nullptr);

        assert_true(a3->parent_is_stage());
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

        assert_close(expected_abs.x, actor2->absolute_rotation().x, 0.000001f);
        assert_close(expected_abs.y, actor2->absolute_rotation().y, 0.000001f);
        assert_close(expected_abs.z, actor2->absolute_rotation().z, 0.000001f);
        assert_close(expected_abs.w, actor2->absolute_rotation().w, 0.000001f);

        assert_close(expected_rel.x, actor2->rotation().x, 0.000001f);
        assert_close(expected_rel.y, actor2->rotation().y, 0.000001f);
        assert_close(expected_rel.z, actor2->rotation().z, 0.000001f);
        assert_close(expected_rel.w, actor2->rotation().w, 0.000001f);
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

    void test_find_descendent_by_name() {
        auto actor1 = stage_->new_actor_with_name("actor1");

        assert_equal(actor1, stage_->find_descendent_with_name("actor1"));
        assert_is_null(stage_->find_descendent_with_name("bananas"));

        auto dupe = stage_->new_actor_with_name("actor1");
        dupe->set_parent(actor1);

        // Find the first one in a leaf-first, left-to-right order
        // (e.g. the dupe)
        assert_equal(dupe, stage_->find_descendent_with_name("actor1"));
    }

    void test_visibility() {
        auto a1 = stage_->new_actor();
        auto a2 = stage_->new_actor_with_parent(a1);
        auto a3 = stage_->new_actor_with_parent(a2);

        a2->set_visible(false);

        assert_true(a1->is_visible());
        assert_false(a2->is_visible());
        assert_false(a3->is_visible());
        assert_false(a2->is_intended_visible());
        assert_true(a3->is_intended_visible());
    }

    void test_actor_findable() {
        auto a1 = stage_->new_actor()->set_name_and_get("Actor 1");
        assert_equal(a1, stage_->find_descendent_with_name("Actor 1"));
    }

    void test_light_findable() {
        auto l1 = stage_->new_light_as_point()->set_name_and_get("Light 1");
        assert_equal(l1, stage_->find_descendent_with_name("Light 1"));
    }

    void test_destroy_after() {
        auto a1 = stage_->new_actor()->set_name_and_get("test");
        a1->destroy_after(smlt::Seconds(0.1));
        assert_is_not_null(stage_->find_descendent_with_name("test"));

        auto t = application->time_keeper->now_in_us();
        while((application->time_keeper->now_in_us() - t) < 150000) {
            application->run_frame();
        }

        assert_is_null(stage_->find_descendent_with_name("test"));
    }

    void test_link_position() {
        auto a1 = stage_->new_actor();
        auto a2 = stage_->new_actor();

        a1->move_to(-50, 0, 0);
        a2->link_position(a1);

        // Should've synced position immediately
        assert_equal(a2->absolute_position(), smlt::Vec3(-50, 0, 0));

        a2->move_to(100, 0, 0);
        a1->move_to(50, 0, 0);

        assert_equal(a1->absolute_position(), smlt::Vec3(50, 0, 0));
        assert_equal(a2->absolute_position(), smlt::Vec3(100, 0, 0));

        /* Should sync the position from a1 */
        a2->late_update(0);
        assert_equal(a2->absolute_position(), smlt::Vec3(50, 0, 0));
    }
private:
    smlt::CameraPtr camera_;
    smlt::StagePtr stage_;
};

}
#endif // TEST_OBJECT_H
