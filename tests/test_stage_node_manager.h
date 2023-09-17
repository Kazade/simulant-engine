#pragma once

#include "simulant/stage.h"
#include "simulant/test.h"
#include "simulant/nodes/camera.h"
#include "simulant/nodes/stage_node_manager.h"
#include "simulant/compositor.h"

namespace {

using namespace smlt;

class MyObject;

typedef smlt::default_init_ptr<MyObject> MyObjectPtr;
typedef StageNodeID MyObjectID;

typedef smlt::StageNodeManager<StageNodePool, StageNodeID, Actor> MyObjectManager;

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
        assert_equal(manager.size(), 1u); // Still there
        assert_true(manager.contains(obj->id()));

        auto id = obj->id();

        manager.clean_up();

        assert_is_null(manager.get(id));
        assert_equal(manager.size(), 0u); // Gone
        assert_false(manager.contains(id));
    }

    void test_clear_immediately_destroys() {

    }

    void test_actors_are_freed() {
        auto stage = scene->new_stage();

        auto count = stage->actor_count();

        auto actor = stage->new_actor();
        assert_equal(actor->node_type(), STAGE_NODE_TYPE_ACTOR);
        assert_equal(stage->actor_count(), count + 1);

        stage->destroy_actor(actor->id());

        // Should be the same, the original actor is still lingering
        assert_equal(stage->actor_count(), count + 1);

        application->run_frame();

        // Back to where we were
        assert_equal(stage->actor_count(), count);
    }

    void test_lights_are_freed() {
        auto stage = scene->new_stage();

        auto count = stage->light_count();

        auto light = stage->new_light_as_directional();
        assert_equal(light->node_type(), STAGE_NODE_TYPE_LIGHT);
        assert_equal(stage->light_count(), count + 1);

        stage->destroy_light(light->id());

        assert_equal(stage->light_count(), count + 1);

        application->run_frame();

        assert_equal(stage->light_count(), count);
    }

    void test_particle_systems_are_freed() {
        auto stage = scene->new_stage();

        auto script = scene->assets->new_particle_script_from_file(
            ParticleScript::BuiltIns::FIRE
        );

        auto count = stage->particle_system_count();

        auto particle_system = stage->new_particle_system(script);
        assert_equal(particle_system->node_type(), STAGE_NODE_TYPE_PARTICLE_SYSTEM);

        assert_equal(stage->particle_system_count(), count + 1);

        stage->destroy_particle_system(particle_system->id());

        assert_equal(stage->particle_system_count(), count + 1);

        application->run_frame();

        assert_equal(stage->particle_system_count(), count);
    }

    void test_geoms_are_freed() {
        auto stage = scene->new_stage();

        auto mesh = scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT);

        auto count = stage->geom_count();

        auto geom = stage->new_geom_with_mesh(mesh);

        assert_equal(geom->node_type(), STAGE_NODE_TYPE_GEOM);
        assert_equal(stage->geom_count(), count + 1);

        stage->destroy_geom(geom->id());

        assert_equal(stage->geom_count(), count + 1);

        application->run_frame();

        assert_equal(stage->geom_count(), count);
    }

    void test_cameras_are_freed() {
        auto stage = scene->new_stage();

        auto count = stage->camera_count();

        auto camera = stage->new_camera();
        assert_equal(camera->node_type(), STAGE_NODE_TYPE_CAMERA);

        assert_equal(stage->camera_count(), count + 1);
        stage->destroy_camera(camera->id());

        assert_equal(stage->camera_count(), count + 1);

        application->run_frame();

        assert_equal(stage->camera_count(), count);
    }

    void test_pipelines_are_freed() {
        auto stage = scene->new_stage();
        auto pipeline = window->compositor->render(stage, stage->new_camera());

        auto name = pipeline->name();
        pipeline->destroy();
        assert_true(window->compositor->has_pipeline(name));

        application->run_frame();
        assert_false(window->compositor->has_pipeline(name));
    }

    void test_stages_are_freed() {
        auto count = scene->stage_count();

        auto stage = scene->new_stage();

        assert_equal(scene->stage_count(), count + 1);

        scene->destroy_stage(stage->id());

        assert_true(stage->is_destroyed());
        assert_equal(scene->stage_count(), count + 1);
        application->run_frame();

        assert_equal(scene->stage_count(), count);
    }

    void test_backgrounds_are_freed() {

    }

    void test_skyboxes_are_freed() {

    }

    void test_widgets_are_freed() {

    }

    void test_sprites_are_freed() {
        auto stage = scene->new_stage();

        auto count = stage->sprites->sprite_count();

        auto sprite = stage->sprites->new_sprite();
        assert_equal(sprite->node_type(), STAGE_NODE_TYPE_SPRITE);

        assert_true(stage->sprites->sprite_count() >= count + 1);

        stage->sprites->destroy_sprite(sprite->id());

        assert_true(stage->sprites->sprite_count() >= count + 1);

        application->run_frame();

        assert_equal(stage->sprites->sprite_count(), count);
    }
};

}
