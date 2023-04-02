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
    StageNodeResult<Actor> child_one = S_FIND_DESCENDENT("Child 1");
    StageNodeResult<ParticleSystem> invalid_child = S_FIND_DESCENDENT("Child 1");

    const char* name() const {
        return "lookups";
    }
};

class BehaviourLookupTests : public test::SimulantTestCase {
public:
    void test_descendent_lookups() {
        auto stage = scene->new_stage();
        auto actor = stage->new_actor();
        auto camera = stage->new_camera();

        auto b = actor->new_behaviour<BehaviourWithLookups>();

        assert_is_null((StageNode*) b->child_one);
        assert_is_null((StageNode*) b->invalid_child);

        auto child_one = stage->new_actor_with_name("Child 1");
        child_one->set_parent(actor);

        assert_is_not_null((StageNode*) b->child_one);
        assert_equal(b->child_one.get(), child_one);
        assert_equal(b->child_one->id(), child_one->id());
        assert_is_null((StageNode*) b->invalid_child);

        child_one->destroy();

        assert_is_null((StageNode*) b->child_one);
        assert_is_null((StageNode*) b->invalid_child);
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
