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

        camera_ = scene->create_child<smlt::Camera>();
    }

    void tear_down() {
        camera_->destroy();
        SimulantTestCase::tear_down();
    }

    void test_move_to_origin() {
        window->compositor->render(scene, camera_);

        // A bug was reported that this caused a crash (see #219)
        auto mesh = scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_cube("cube", scene->assets->new_material(), 50.0f);

        auto actor = scene->create_child<smlt::Actor>(mesh);
        actor->transform->set_translation(Vec3(0, 0, 0));
        application->run_frame();
    }

    void test_move_forward_by() {
        auto actor = scene->create_child<smlt::Stage>();

        actor->transform->set_rotation(smlt::Quaternion(smlt::Degrees(0), smlt::Degrees(90), smlt::Degrees(0)));
        actor->transform->translate(actor->transform->forward() * 200);

        assert_close(actor->transform->translation().x, -200.0f, 0.0001f);
        assert_close(actor->transform->translation().y, 0.0f, 0.0001f);
        assert_close(actor->transform->translation().z, 0.0f, 0.0001f);
    }

    void test_scaling_applies() {
        auto actor = scene->create_child<smlt::Stage>();
        actor->transform->set_scale_factor(smlt::Vec3(2.0, 1.0, 0.5));

        auto transform = actor->transform->world_space_matrix();

        assert_equal(transform[0], 2.0f);
        assert_equal(transform[5], 1.0f);
        assert_equal(transform[10], 0.5f);

        auto actor2 = scene->create_child<smlt::Stage>();

        actor2->transform->rotate(smlt::Vec3::POSITIVE_Y, smlt::Degrees(1.0));

        auto first = actor2->transform->world_space_matrix();

        actor2->transform->set_scale_factor(smlt::Vec3(2.0, 2.0, 2.0));

        auto second = actor2->transform->world_space_matrix();

        assert_equal(first[0] * 2, second[0]);
        assert_equal(first[5] * 2, second[5]);
        assert_equal(first[10] * 2, second[10]);
    }

    void test_set_parent() {
        auto actor1 = scene->create_child<smlt::Stage>();
        auto actor2 = scene->create_child<smlt::Stage>();

        actor2->set_parent(actor1);

        actor1->transform->set_translation(Vec3(0, 10, 0));

        assert_equal(Vec3(0, 10, 0), actor2->transform->position());
        assert_equal(Vec3(), actor2->transform->translation());

        actor1->transform->set_rotation(Quaternion(Degrees(0), Degrees(90), Degrees(0)));

        assert_close(actor2->transform->orientation().x, actor1->transform->orientation().x, 0.00001f);
        assert_close(actor2->transform->orientation().y, actor1->transform->orientation().y, 0.00001f);
    }

    void test_parent_is_scene() {
        auto a1 = scene->create_child<smlt::Stage>();
        auto a2 = scene->create_child<smlt::Stage>();
        auto a3 = scene->create_child<smlt::Stage>();

        a2->set_parent(a1);
        a3->set_parent(a2);

        assert_true(a1->parent_is_scene());
        assert_false(a2->parent_is_scene());
        assert_false(a3->parent_is_scene());

        a3->set_parent(nullptr);

        assert_false(a3->parent_is_scene());
        auto strays = scene->stray_nodes();
        assert_true(std::find(strays.begin(), strays.end(), a3) != strays.end());
    }

    void test_parent_transformation_applied() {
        auto actor1 = scene->create_child<smlt::Stage>();
        auto actor2 = scene->create_child<smlt::Stage>();
        actor2->set_parent(actor1);

        actor2->transform->set_translation(smlt::Vec3(0, 0, -5));

        actor1->transform->set_translation(smlt::Vec3(0, 0, -5));
        actor1->transform->set_rotation(smlt::Quaternion(smlt::Degrees(0), smlt::Degrees(90), smlt::Degrees(0)));

        assert_close(actor2->transform->position().x, -5.0f, 0.00001f);
        assert_close(actor2->transform->position().y,  0.0f, 0.00001f);
        assert_close(actor2->transform->position().z, -5.0f, 0.00001f);
    }

    void test_set_absolute_rotation() {
        auto actor = scene->create_child<smlt::Stage>();

        actor->transform->set_orientation(smlt::Quaternion(smlt::Vec3(0, 0, 1), smlt::Degrees(10)));

        assert_equal(actor->transform->rotation(), actor->transform->orientation());

        auto actor2 = scene->create_child<smlt::Stage>();

        actor2->set_parent(actor);

        assert_equal(actor2->transform->orientation(), actor->transform->orientation());

        actor2->transform->set_orientation(smlt::Quaternion(smlt::Vec3(0, 0, 1), smlt::Degrees(20)));

        smlt::Quaternion expected_rel(smlt::Vec3::POSITIVE_Z, smlt::Degrees(10));
        smlt::Quaternion expected_abs(smlt::Vec3::POSITIVE_Z, smlt::Degrees(20));

        assert_close(expected_abs.x, actor2->transform->orientation().x, 0.000001f);
        assert_close(expected_abs.y, actor2->transform->orientation().y, 0.000001f);
        assert_close(expected_abs.z, actor2->transform->orientation().z, 0.000001f);
        assert_close(expected_abs.w, actor2->transform->orientation().w, 0.000001f);

        assert_close(expected_rel.x, actor2->transform->rotation().x, 0.000001f);
        assert_close(expected_rel.y, actor2->transform->rotation().y, 0.000001f);
        assert_close(expected_rel.z, actor2->transform->rotation().z, 0.000001f);
        assert_close(expected_rel.w, actor2->transform->rotation().w, 0.000001f);
    }

    void test_set_absolute_position() {
        auto actor = scene->create_child<smlt::Stage>();

        actor->transform->set_position(Vec3(10, 10, 10));

        assert_equal(smlt::Vec3(10, 10, 10), actor->transform->translation());

        auto actor2 = scene->create_child<smlt::Stage>();

        actor2->set_parent(actor);

        //Should be the same as its parent
        assert_equal(actor2->transform->position(), actor->transform->position());

        //Make sure relative position is correctly calculated
        actor2->transform->set_position(Vec3(20, 10, 10));
        assert_equal(smlt::Vec3(10, 0, 0), actor2->transform->translation());
        assert_equal(smlt::Vec3(20, 10, 10), actor2->transform->position());

        //Make sure setting by vec3 works
        actor2->transform->set_position(smlt::Vec3(0, 0, 0));
        assert_equal(smlt::Vec3(), actor2->transform->position());
    }

    void test_set_relative_position() {
        auto actor = scene->create_child<smlt::Stage>();

        actor->transform->set_translation(Vec3(10, 10, 10));

        //No parent, both should be the same
        assert_equal(smlt::Vec3(10, 10, 10), actor->transform->translation());
        assert_equal(smlt::Vec3(10, 10, 10), actor->transform->position());

        auto actor2 = scene->create_child<smlt::Stage>();

        actor2->set_parent(actor);

        actor2->transform->set_translation(smlt::Vec3(10, 0, 0));

        assert_equal(smlt::Vec3(20, 10, 10), actor2->transform->position());
        assert_equal(smlt::Vec3(10, 0, 0), actor2->transform->translation());
    }

    void test_move_updates_children() {
        auto actor1 = scene->create_child<smlt::Stage>();
        auto actor2 = scene->create_child<smlt::Stage>();

        actor2->transform->set_translation(Vec3(0, 0, 10.0f));
        actor2->set_parent(actor1);

        assert_equal(10.0f, actor2->transform->translation().z);
    }

    void test_set_parent_to_self_does_nothing() {
        auto actor1 = scene->create_child<smlt::Stage>();

        auto original_parent = actor1->parent();
        actor1->set_parent(actor1);
        assert_equal(original_parent, actor1->parent());
    }

    void test_find_descendent_by_name() {
        auto actor1 = scene->create_child<smlt::Stage>();
        actor1->set_name("actor1");

        assert_equal(actor1, scene->find_descendent_with_name("actor1"));
        assert_is_null(scene->find_descendent_with_name("bananas"));

        auto dupe = scene->create_child<smlt::Stage>();
        dupe->set_name("actor1");
        dupe->set_parent(actor1);

        // Find the first one in a leaf-first, left-to-right order
        // (e.g. the dupe)
        assert_equal(dupe, scene->find_descendent_with_name("actor1"));
    }

    void test_visibility() {
        auto a1 = scene->create_child<smlt::Stage>();
        auto a2 = scene->create_child<smlt::Stage>();
        a2->set_parent(a1);

        auto a3 = scene->create_child<smlt::Stage>();
        a3->set_parent(a2);
        a2->set_visible(false);

        assert_true(a1->is_visible());
        assert_false(a2->is_visible());
        assert_false(a3->is_visible());
        assert_false(a2->is_intended_visible());
        assert_true(a3->is_intended_visible());
    }

    void test_actor_findable() {
        auto a1 = scene->create_child<smlt::Stage>();
        a1->set_name("Actor 1");
        assert_equal(a1, scene->find_descendent_with_name("Actor 1"));
    }

    void test_light_findable() {
        auto l1 = scene->create_child<PointLight>();
        l1->set_name("Light 1");
        assert_equal(l1, scene->find_descendent_with_name("Light 1"));
    }

    void test_destroy_after() {
        auto a1 = scene->create_child<smlt::Stage>();
        a1->set_name("test");
        auto p = a1->destroy_after(smlt::Seconds(0.1));

        assert_false(p.is_ready());
        assert_is_not_null(scene->find_descendent_with_name("test"));

        auto t = application->time_keeper->now_in_us();
        while((application->time_keeper->now_in_us() - t) < 150000) {
            application->run_frame();
        }

        assert_is_null(scene->find_descendent_with_name("test"));
        assert_true(p.is_ready());
    }

private:
    smlt::CameraPtr camera_;
};

}
#endif // TEST_OBJECT_H
