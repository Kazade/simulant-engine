#pragma once

#include "simulant/generic/manual_manager.h"
#include "simulant/generic/unique_id.h"
#include "simulant/stage.h"
#include "simulant/test.h"
#include "simulant/nodes/camera.h"

namespace {

using namespace smlt;

class MyObject;

typedef smlt::default_init_ptr<MyObject> MyObjectPtr;
typedef smlt::UniqueID<MyObjectPtr> MyObjectID;

typedef smlt::ManualManager<MyObject, MyObjectID> MyObjectManager;

class MyObject {
public:
    MyObjectID id_;

    MyObject(MyObjectID id): id_(id) {}

    bool init() { return true; }
    void clean_up() {}
    void _bind_id_pointer(MyObject*) {}
};

class ManualManagerTests : public smlt::test::SimulantTestCase {
public:

    void test_basic_usage() {
        MyObjectManager manager;
        MyObject* obj = manager.make();
        auto id = obj->id_;

        assert_is_not_null(obj);
        assert_true(obj->id_);
        assert_true(manager.contains(obj->id_));
        assert_equal(manager.size(), 1u);
        assert_equal(manager.capacity(), MyObjectManager::chunk_size);
        assert_is_not_null(manager.get(obj->id_));

        manager.destroy(obj->id_);

        assert_is_not_null(manager.get(obj->id_));
        assert_true(manager.is_marked_for_destruction(obj->id_));
        assert_equal(manager.size(), 1u); // Still there
        assert_true(manager.contains(obj->id_));

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

        auto actor = stage->new_actor()->id();
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

        auto light = stage->new_light_as_directional()->id();
        stage->destroy_light(light);

        // Should be different, the original light is still lingering
        assert_not_equal(stage->new_light_as_directional()->id(), light);

        window->run_frame();

        // Same ID should be given back as it's been released
        assert_equal(stage->new_light_as_directional()->id(), light);
    }

    void test_particle_systems_are_freed() {
        auto stage = window->new_stage();

        auto script = stage->assets->new_particle_script_from_file(
            ParticleScript::BuiltIns::FIRE
        );

        auto particle_system = stage->new_particle_system(script)->id();
        stage->destroy_particle_system(particle_system);

        // Should be different, the original particle system is still lingering
        assert_not_equal(stage->new_particle_system(script)->id(), particle_system);

        window->run_frame();

        // Same ID should be given back as it's been released
        assert_equal(stage->new_particle_system(script)->id(), particle_system);
    }

    void test_geoms_are_freed() {
        auto stage = window->new_stage();

        auto mesh = stage->assets->new_mesh(smlt::VertexSpecification::DEFAULT);

        auto geom = stage->new_geom_with_mesh(mesh)->id();
        stage->destroy_geom(geom);

        // Should be different, the original light is still lingering
        assert_not_equal(stage->new_geom_with_mesh(mesh)->id(), geom);

        window->run_frame();

        // Same ID should be given back as it's been released
        assert_equal(stage->new_geom_with_mesh(mesh)->id(), geom);
    }

    void test_cameras_are_freed() {
        auto stage = window->new_stage();

        auto count = stage->node_pool->size();

        auto camera = stage->new_camera()->id();

        assert_equal(stage->node_pool->size(), count + 1);
        stage->destroy_camera(camera);

        assert_equal(stage->node_pool->size(), count + 1);

        window->run_frame();

        assert_equal(stage->node_pool->size(), count);
    }

    void test_pipelines_are_freed() {
        auto stage = window->new_stage();
        auto pipeline = window->render(stage, stage->new_camera()).id();

        window->destroy_pipeline(pipeline);
        assert_true(window->has_pipeline(pipeline));

        window->run_frame();
        assert_false(window->has_pipeline(pipeline));
    }

    void test_stages_are_freed() {
        auto stage = window->new_stage()->id();

        window->destroy_stage(stage);

        // Should be different, the original light is still lingering
        assert_not_equal(window->new_stage()->id(), stage);

        window->run_frame();

        // Same ID should be given back as it's been released
        assert_equal(window->new_stage()->id(), stage);
    }

    void test_backgrounds_are_freed() {

    }

    void test_skyboxes_are_freed() {

    }

    void test_widgets_are_freed() {

    }

    void test_sprites_are_freed() {
        auto stage = window->new_stage();

        auto sprite = stage->sprites->new_sprite();
        stage->sprites->destroy_sprite(sprite);

        // Should be different, the original light is still lingering
        assert_not_equal(stage->sprites->new_sprite()->id(), sprite);

        window->run_frame();

        // Same ID should be given back as it's been released
        assert_equal(stage->sprites->new_sprite()->id(), sprite);
    }
};

}
