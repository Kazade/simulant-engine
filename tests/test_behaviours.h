#pragma once

#include <simulant/test.h>
#include <simulant/simulant.h>

#include "../simulant/behaviours/movement/cylindrical_billboard.h"
#include "../simulant/behaviours/movement/spherical_billboard.h"
#include "../simulant/behaviours/locators/node_locator.h"

namespace {

using namespace smlt;

class BehaviourWithLookups : public Behaviour, public RefCounted<BehaviourWithLookups> {
public:
    FindResult<Actor> child_one = FindDescendent("Child 1", this);
    FindResult<ParticleSystem> invalid_child = FindDescendent("Child 1", this);
    FindResult<Camera> camera = FindDescendent("Camera", this);

    FindResult<Actor> parent = FindAncestor("Some Parent", this);

    const char* name() const {
        return "lookups";
    }
};

class BehaviourLookupTests : public test::SimulantTestCase {
public:
    void test_ancestor_lookups() {
        auto stage = scene->new_stage();
        auto actor = stage->new_actor();
        auto b = actor->new_behaviour<BehaviourWithLookups>();

        assert_is_null((StageNode*) b->parent.get());

        auto parent = stage->new_actor();
        parent->set_name("Some Parent");
        actor->set_parent(parent);

        assert_equal((StageNode*) b->parent.get(), (StageNode*) parent);
    }

    void test_descendent_lookups() {
        auto stage = scene->new_stage();
        auto actor = stage->new_actor();
        auto camera = stage->new_camera();

        auto b = actor->new_behaviour<BehaviourWithLookups>();

        assert_is_null((StageNode*) b->child_one);
        assert_is_null((StageNode*) b->invalid_child);

        auto child_one = stage->new_actor_with_name("Child 1");
        child_one->set_parent(actor);

        camera->set_parent(child_one);
        camera->set_name("Camera");

        assert_is_not_null((StageNode*) b->child_one);
        assert_equal((StageNode*) b->child_one.get(), (StageNode*) child_one);
        assert_equal(b->child_one->id(), child_one->id());
        assert_is_null((StageNode*) b->invalid_child);

        child_one->destroy();

        assert_false(b->child_one.is_cached());
        assert_false(b->invalid_child.is_cached());
    }
};



class CylindricalBillboardTests : public test::SimulantTestCase {
public:
    void test_basic_usage() {
        auto stage = scene->new_stage();
        auto actor = stage->new_actor();
        auto camera = stage->new_camera();

        auto pipeline = window->compositor->render(stage, camera);
        pipeline->activate();

        actor->new_behaviour<behaviours::CylindricalBillboard>(camera);

        camera->move_to(0, 0, 100);

        application->run_frame();
        assert_equal(actor->forward(), Vec3(0, 0, 1));

        camera->move_to(0, 100, 0);

        application->run_frame();

        // Default to negative Z
        assert_equal(actor->forward(), Vec3(0, 0, -1));

        camera->move_to(100, 0, 0);

        application->run_frame();
        assert_equal(actor->forward(), Vec3(1, 0, 0));
    }
};

class SphericalBillboardTests : public test::SimulantTestCase {
public:

    void test_basic_usage() {
        auto stage = scene->new_stage();
        auto actor = stage->new_actor();
        auto camera = stage->new_camera();

        auto pipeline = window->compositor->render(stage, camera);
        pipeline->activate();

        actor->new_behaviour<behaviours::SphericalBillboard>(camera);

        camera->move_to(0, 0, 100);

        application->run_frame();
        assert_equal(actor->forward(), Vec3(0, 0, 1));

        camera->move_to(0, 100, 0);

        application->run_frame();
        assert_equal(actor->forward(), Vec3(0, 1, 0));

        camera->move_to(100, 0, 0);

        application->run_frame();
        assert_equal(actor->forward(), Vec3(1, 0, 0));
    }
};

}
