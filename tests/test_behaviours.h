#pragma once

#include <simulant/test.h>
#include <simulant/simulant.h>

#include "../simulant/behaviours/locators/node_locator.h"

namespace {

using namespace smlt;

struct NodeWithLookupsParams {};

class NodeWithLookups : public StageNode {
public:
    struct Meta {
        const static StageNodeType node_type = (STAGE_NODE_TYPE_USER_BASE + 1);
        typedef NodeWithLookupsParams params_type;
    };

    NodeWithLookups(Scene* owner):
        StageNode(owner, STAGE_NODE_TYPE_USER_BASE + 1) {}

    FindResult<Actor> child_one = FindDescendent("Child 1", this);
    FindResult<ParticleSystem> invalid_child = FindDescendent("Child 1", this);
    FindResult<Camera> camera = FindDescendent("Camera", this);

    FindResult<Actor> parent = FindAncestor("Some Parent", this);

    bool on_create(void*) { return true; }
    void do_generate_renderables(batcher::RenderQueue *, const Camera *, const Viewport *, const DetailLevel) override {}
    const AABB& aabb() const {
        static AABB aabb;
        return aabb;
    }
};

class BehaviourLookupTests : public test::SimulantTestCase {
public:
    void set_up() {
        test::SimulantTestCase::set_up();
        scene->register_stage_node<NodeWithLookups>();
    }

    void test_ancestor_lookups() {
        auto m = scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        auto b = scene->create_child<NodeWithLookups>();

        assert_is_null((StageNode*) b->parent.get());

        auto parent = scene->create_child<smlt::Actor>(m);
        parent->set_name("Some Parent");
        b->set_parent(parent);

        assert_equal((StageNode*) b->parent.get(), (StageNode*) parent);
    }

    void test_descendent_lookups() {
        auto m = scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        auto camera = scene->create_child<smlt::Camera>();

        auto b = scene->create_child<NodeWithLookups>();

        assert_is_null((StageNode*) b->child_one);
        assert_is_null((StageNode*) b->invalid_child);

        auto child_one = scene->create_child<Actor>(m);
        child_one->set_name("Child 1");
        child_one->set_parent(b);

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
        auto actor = scene->create_child<smlt::Stage>();
        auto camera = scene->create_child<smlt::Camera>();

        auto pipeline = window->compositor->render(scene, camera);
        pipeline->activate();

        auto billboard = scene->create_child<CylindricalBillboard>();
        billboard->set_target(camera);
        billboard->adopt_children(actor);

        camera->transform->set_translation(Vec3(0, 0, 100));

        application->run_frame();
        assert_equal(actor->transform->forward(), Vec3(0, 0, 1));

        camera->transform->set_translation(Vec3(0, 100, 0));

        application->run_frame();

        // Default to negative Z
        assert_equal(actor->transform->forward(), Vec3(0, 0, -1));

        camera->transform->set_translation(Vec3(100, 0, 0));

        application->run_frame();
        assert_equal(actor->transform->forward(), Vec3(1, 0, 0));
    }
};

class SphericalBillboardTests : public test::SimulantTestCase {
public:

    void test_basic_usage() {
        auto actor = scene->create_child<smlt::Stage>();
        auto camera = scene->create_child<smlt::Camera>();

        auto pipeline = window->compositor->render(scene, camera);
        pipeline->activate();

        auto billboard = scene->create_child<SphericalBillboard>();
        billboard->set_target(camera);
        billboard->adopt_children(actor);

        camera->transform->set_translation(Vec3(0, 0, 100));

        application->run_frame();
        assert_equal(actor->transform->forward(), Vec3(0, 0, 1));

        camera->transform->set_translation(Vec3(0, 100, 0));

        application->run_frame();
        assert_equal(actor->transform->forward(), Vec3(0, 1, 0));

        camera->transform->set_translation(Vec3(100, 0, 0));

        application->run_frame();
        assert_equal(actor->transform->forward(), Vec3(1, 0, 0));
    }
};

}
