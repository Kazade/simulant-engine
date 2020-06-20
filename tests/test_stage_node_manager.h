#pragma once

#include "simulant/generic/unique_id.h"
#include "simulant/stage.h"
#include "simulant/test.h"
#include "simulant/nodes/camera.h"
#include "simulant/nodes/stage_node_manager.h"
#include "simulant/compositor.h"

namespace {

using namespace smlt;

class MyObject;

typedef smlt::default_init_ptr<MyObject> MyObjectPtr;
typedef smlt::UniqueID<MyObjectPtr> MyObjectID;

typedef smlt::StageNodeManager<StageNodePool, ActorID, Actor> MyObjectManager;

class StageNodeManagerTests : public smlt::test::SimulantTestCase {
public:

    void test_basic_usage() {
        StageNodePool pool(10);
        MyObjectManager manager(&pool);
        Actor* obj = manager.make(nullptr, nullptr);

        assert_is_not_null(obj);
        assert_true(manager.contains(obj->id()));
        assert_equal(manager.size(), 1u);
        assert_equal(pool.capacity(), 10u);
        assert_is_not_null(manager.get(obj->id()));

        manager.destroy(obj->id());

        assert_is_not_null(manager.get(obj->id()));
        assert_true(manager.is_marked_for_destruction(obj->id()));
        assert_equal(manager.size(), 1u); // Still there
        assert_true(manager.contains(obj->id()));

        auto id = obj->id();

        manager.clean_up();

        assert_is_null(manager.get(id));
        assert_false(manager.is_marked_for_destruction(id));
        assert_equal(manager.size(), 0u); // Gone
        assert_false(manager.contains(id));
    }

    void test_clear_immediately_destroys() {

    }

    void test_actors_are_freed() {
        auto stage = window->new_stage();

        auto count = stage->node_pool->size();

        auto actor = stage->new_actor();
        assert_equal(actor->node_type(), STAGE_NODE_TYPE_ACTOR);
        assert_equal(stage->node_pool->size(), count + 1);

        stage->destroy_actor(actor);

        // Should be the same, the original actor is still lingering
        assert_equal(stage->node_pool->size(), count + 1);

        window->run_frame();

        // Back to where we were
        assert_equal(stage->node_pool->size(), count);
    }

    void test_lights_are_freed() {
        auto stage = window->new_stage();

        auto count = stage->node_pool->size();

        auto light = stage->new_light_as_directional();
        assert_equal(light->node_type(), STAGE_NODE_TYPE_LIGHT);
        assert_equal(stage->node_pool->size(), count + 1);

        stage->destroy_light(light);

        assert_equal(stage->node_pool->size(), count + 1);

        window->run_frame();

        assert_equal(stage->node_pool->size(), count);
    }

    void test_particle_systems_are_freed() {
        auto stage = window->new_stage();

        auto script = stage->assets->new_particle_script_from_file(
            ParticleScript::BuiltIns::FIRE
        );

        auto count = stage->node_pool->size();

        auto particle_system = stage->new_particle_system(script);
        assert_equal(particle_system->node_type(), STAGE_NODE_TYPE_PARTICLE_SYSTEM);

        assert_equal(stage->node_pool->size(), count + 1);

        stage->destroy_particle_system(particle_system);

        assert_equal(stage->node_pool->size(), count + 1);

        window->run_frame();

        assert_equal(stage->node_pool->size(), count);
    }

    void test_geoms_are_freed() {
        auto stage = window->new_stage();

        auto mesh = stage->assets->new_mesh(smlt::VertexSpecification::DEFAULT);

        auto count = stage->node_pool->size();

        auto geom = stage->new_geom_with_mesh(mesh);

        assert_equal(geom->node_type(), STAGE_NODE_TYPE_GEOM);
        assert_equal(stage->node_pool->size(), count + 1);

        stage->destroy_geom(geom);

        assert_equal(stage->node_pool->size(), count + 1);

        window->run_frame();

        assert_equal(stage->node_pool->size(), count);
    }

    void test_cameras_are_freed() {
        auto stage = window->new_stage();

        auto count = stage->node_pool->size();

        auto camera = stage->new_camera();
        assert_equal(camera->node_type(), STAGE_NODE_TYPE_CAMERA);

        assert_equal(stage->node_pool->size(), count + 1);
        stage->destroy_camera(camera);

        assert_equal(stage->node_pool->size(), count + 1);

        window->run_frame();

        assert_equal(stage->node_pool->size(), count);
    }

    void test_pipelines_are_freed() {
        auto stage = window->new_stage();
        auto pipeline = window->compositor->render(stage, stage->new_camera());

        auto name = pipeline->name();
        pipeline->destroy();
        assert_true(window->compositor->has_pipeline(name));

        window->run_frame();
        assert_false(window->compositor->has_pipeline(name));
    }

    void test_stages_are_freed() {
        auto count = window->pool_.size();

        auto stage = window->new_stage()->id();

        assert_equal(window->pool_.size(), count + 1);

        window->destroy_stage(stage);

        assert_equal(window->pool_.size(), count + 1);

        window->run_frame();

        assert_equal(window->pool_.size(), count);
    }

    void test_backgrounds_are_freed() {

    }

    void test_skyboxes_are_freed() {

    }

    void test_widgets_are_freed() {

    }

    void test_sprites_are_freed() {
        auto stage = window->new_stage();

        auto count = stage->node_pool->size();

        auto sprite = stage->sprites->new_sprite();
        assert_equal(sprite->node_type(), STAGE_NODE_TYPE_OTHER);

        assert_true(stage->node_pool->size() >= count + 1);

        stage->sprites->destroy_sprite(sprite);

        assert_true(stage->node_pool->size() >= count + 1);

        window->run_frame();

        assert_equal(stage->node_pool->size(), count);
    }
};

}
