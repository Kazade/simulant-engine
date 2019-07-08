#pragma once

#include "simulant/generic/manual_manager.h"
#include "simulant/generic/unique_id.h"
#include "simulant/test.h"

class MyObject;

typedef smlt::default_init_ptr<MyObject> MyObjectPtr;
typedef smlt::UniqueID<MyObjectPtr> MyObjectID;

typedef smlt::ManualManager<MyObject, MyObjectID> MyObjectManager;

class MyObject {
public:
    MyObjectID id_;

    MyObject(MyObjectID id): id_(id) {}

    bool init() { return true; }
    void cleanup() {}
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

        auto actor = stage->new_actor()->id();
        stage->delete_actor(actor);

        // Should be different, the original actor is still lingering
        assert_not_equal(stage->new_actor()->id(), actor);

        window->run_frame();

        // Same ID should be given back as it's been released
        assert_equal(stage->new_actor()->id(), actor);
    }

    void test_lights_are_freed() {

    }

    void test_particle_systems_are_freed() {

    }

    void test_geoms_are_freed() {

    }

    void test_cameras_are_freed() {

    }

    void test_pipelines_are_freed() {

    }

    void test_stages_are_freed() {

    }

    void test_backgrounds_are_freed() {

    }

    void test_skyboxes_are_freed() {

    }

    void test_widgets_are_freed() {

    }

    void test_sprites_are_freed() {

    }
};
