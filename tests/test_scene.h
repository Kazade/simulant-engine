#ifndef TEST_sceneS_H
#define TEST_sceneS_H

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/macros.h"

namespace {

using namespace smlt;


class SceneTests : public smlt::test::SimulantTestCase {
public:
    void test_load() {

    }

    void test_unload() {

    }
};

class TestScene : public Scene {
public:
    TestScene(Window* window):
        Scene(window) {}

    void load() override { load_called = true; }
    void unload() override { unload_called = true; }
    void activate() override { activate_called = true; }
    void deactivate() override { deactivate_called = true; }

    volatile bool load_called = false;
    volatile bool unload_called = false;
    volatile bool activate_called = false;
    volatile bool deactivate_called = false;
};

class PreloadArgsScene : public Scene {
public:
    PreloadArgsScene(Window* window):
        Scene(window) {}

    void load() override {
        int arg0 = get_load_arg<int>(0);
        if(arg0 != 99) {
            throw test::AssertionError("Wrong value");
        }
        load_called = true;
    }
    void unload() override { unload_called = true; }
    void activate() override { activate_called = true; }
    void deactivate() override { deactivate_called = true; }

    volatile bool load_called = false;
    volatile bool unload_called = false;
    volatile bool activate_called = false;
    volatile bool deactivate_called = false;
};

class SceneWithArgs : public Scene {
public:
    // Boilerplate
    SceneWithArgs(smlt::Window* window, const std::string& some_arg):
        smlt::Scene(window) {

        _S_UNUSED(some_arg);
    }

    void load() {}

};


class SceneManagerTests : public smlt::test::SimulantTestCase {
private:
    SceneManager::ptr manager_;

public:
    void set_up() {
        SimulantTestCase::set_up();
        manager_ = std::make_shared<SceneManager>(window);
        manager_->init();
    }

    void tear_down() {
        manager_->clean_up();
        manager_.reset();
    }

    void test_route() {
        assert_false(manager_->has_scene("main"));
        assert_false(manager_->has_scene("/test"));

        manager_->register_scene<TestScene>("/test");

        assert_true(manager_->has_scene("/test"));
        assert_false(manager_->has_scene("main"));
    }

    void test_activate() {
        bool signal_called = false;
        auto sig_cb = [&]() {
            signal_called = true;
        };

        auto cb = [this]() {
            manager_->activate("main");
        };

        assert_raises(std::logic_error, cb);

        manager_->register_scene<TestScene>("main");

        TestScene* scr = dynamic_cast<TestScene*>(manager_->resolve_scene("main").get());
        scr->signal_activated().connect(sig_cb);

        manager_->activate("main");
        manager_->late_update(1.0f);

        assert_true(signal_called);

        scr->set_destroy_on_unload(false); //Don't destroy on unload        

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->activate("main"); //activateing to the same place should do nothing
        manager_->late_update(1.0f);

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->register_scene<TestScene>("/test");
        manager_->activate("/test");
        manager_->late_update(1.0f);

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_true(scr->deactivate_called); //Should've been deactivated
        assert_true(scr->unload_called);

        TestScene* scr2 = dynamic_cast<TestScene*>(manager_->resolve_scene("/test").get());

        assert_true(scr2->load_called);
        assert_true(scr2->activate_called);
        assert_false(scr2->deactivate_called);
        assert_false(scr2->unload_called);
    }

    void test_background_load() {
        manager_->register_scene<TestScene>("main");

        TestScene* scr = dynamic_cast<TestScene*>(manager_->resolve_scene("main").get());
        assert_false(scr->load_called);
        manager_->preload_in_background("main").then([this]() {
            manager_->activate("main");
        });
        application->run_frame();
        assert_true(scr->load_called);
    }

    void test_preload_args() {
        manager_->register_scene<PreloadArgsScene>("main");
        manager_->activate(
            "main",
            99 // Additional argument
        );
    }

    void test_unload() {

    }

    void test_is_loaded() {

    }

    void test_reset() {

    }

    void test_register_scene() {
        SceneManager manager(window);
        manager.init();
        manager.register_scene<SceneWithArgs>("test", "arg");

        manager.clean_up();
    }

    void test_actors_are_freed() {
        auto count = scene->count_nodes_by_type<smlt::Stage>();
        auto actor = scene->create_node<smlt::Stage>();
        assert_equal(actor->node_type(), STAGE_NODE_TYPE_STAGE);
        assert_equal(scene->count_nodes_by_type<smlt::Stage>(), count + 1);

        actor->destroy();
        // Should be the same, the original actor is still lingering
        assert_equal(scene->count_nodes_by_type<smlt::Stage>(), count + 1);

        application->run_frame();

        // Back to where we were
        assert_equal(scene->count_nodes_by_type<smlt::Stage>(), count);
    }

    void test_lights_are_freed() {
        auto count = scene->count_nodes_by_type<DirectionalLight>();

        auto light = scene->create_node<DirectionalLight>();
        assert_equal(light->node_type(), STAGE_NODE_TYPE_DIRECTIONAL_LIGHT);
        assert_equal(scene->count_nodes_by_type<DirectionalLight>(), count + 1);

        light->destroy();

        assert_equal(scene->count_nodes_by_type<DirectionalLight>(), count + 1);

        application->run_frame();

        assert_equal(scene->count_nodes_by_type<DirectionalLight>(), count);
    }

    void test_particle_systems_are_freed() {
        auto script = scene->assets->new_particle_script_from_file(
            ParticleScript::BuiltIns::FIRE
        );

        auto count = scene->count_nodes_by_type<ParticleSystem>();

        auto particle_system = scene->create_node<ParticleSystem>(script);
        assert_equal(particle_system->node_type(), STAGE_NODE_TYPE_PARTICLE_SYSTEM);

        assert_equal(scene->count_nodes_by_type<ParticleSystem>(), count + 1);

        particle_system->destroy();

        assert_equal(scene->count_nodes_by_type<ParticleSystem>(), count + 1);

        application->run_frame();

        assert_equal(scene->count_nodes_by_type<ParticleSystem>(), count);
    }

    void test_geoms_are_freed() {
        auto mesh = scene->assets->new_mesh(smlt::VertexSpecification::DEFAULT);

        auto count = scene->count_nodes_by_type<Geom>();

        auto geom = scene->create_node<Geom>(mesh);

        assert_equal(geom->node_type(), STAGE_NODE_TYPE_GEOM);
        assert_equal(scene->count_nodes_by_type<Geom>(), count + 1);

        geom->destroy();

        assert_equal(scene->count_nodes_by_type<Geom>(), count + 1);

        application->run_frame();

        assert_equal(scene->count_nodes_by_type<Geom>(), count);
    }

    void test_cameras_are_freed() {
        auto count = scene->count_nodes_by_type<Camera>();

        auto camera = scene->create_node<smlt::Camera>();
        assert_equal(camera->node_type(), STAGE_NODE_TYPE_CAMERA);

        assert_equal(scene->count_nodes_by_type<Camera>(), count + 1);
        camera->destroy();

        assert_equal(scene->count_nodes_by_type<Camera>(), count + 1);

        application->run_frame();

        assert_equal(scene->count_nodes_by_type<Camera>(), count);
    }

    void test_pipelines_are_freed() {
        auto stage = scene->create_node<smlt::Stage>();
        auto pipeline = window->compositor->render(stage, scene->create_node<smlt::Camera>());

        auto name = pipeline->name();
        pipeline->destroy();
        assert_true(window->compositor->has_pipeline(name));

        application->run_frame();
        assert_false(window->compositor->has_pipeline(name));
    }

    void test_stages_are_freed() {
        auto count = scene->count_nodes_by_type<Stage>();

        auto stage = scene->create_node<smlt::Stage>();

        assert_equal(scene->count_nodes_by_type<Stage>(), count + 1);

        stage->destroy();

        assert_true(stage->is_destroyed());
        assert_equal(scene->count_nodes_by_type<Stage>(), count + 1);
        application->run_frame();

        assert_equal(scene->count_nodes_by_type<Stage>(), count);
    }

    void test_backgrounds_are_freed() {

    }

    void test_skyboxes_are_freed() {

    }

    void test_widgets_are_freed() {

    }

    void test_sprites_are_freed() {
        auto count = scene->count_nodes_by_type<Sprite>();

        auto sprite = scene->create_node<Sprite>();
        assert_equal(sprite->node_type(), STAGE_NODE_TYPE_SPRITE);

        assert_true(scene->count_nodes_by_type<Sprite>() >= count + 1);

        sprite->destroy();

        assert_true(scene->count_nodes_by_type<Sprite>() >= count + 1);

        application->run_frame();

        assert_equal(scene->count_nodes_by_type<Sprite>(), count);
    }
};


}

#endif // TEST_sceneS_H

