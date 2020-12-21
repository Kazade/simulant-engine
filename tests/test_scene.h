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

class TestScene : public Scene<TestScene> {
public:
    TestScene(Window* window):
        Scene<TestScene>(window) {}

    void load() override { load_called = true; }
    void unload() override { unload_called = true; }
    void activate() override { activate_called = true; }
    void deactivate() override { deactivate_called = true; }

    volatile bool load_called = false;
    volatile bool unload_called = false;
    volatile bool activate_called = false;
    volatile bool deactivate_called = false;
};

class PreloadArgsScene : public Scene<PreloadArgsScene> {
public:
    PreloadArgsScene(Window* window):
        Scene<PreloadArgsScene>(window) {}

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

class SceneWithArgs : public Scene<SceneWithArgs> {
public:
    // Boilerplate
    SceneWithArgs(smlt::Window* window, const std::string& some_arg):
        smlt::Scene<SceneWithArgs>(window) {

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
    }

    void test_route() {
        assert_false(manager_->has_scene("main"));
        assert_false(manager_->has_scene("/test"));

        manager_->register_scene<TestScene>("/test");

        assert_true(manager_->has_scene("/test"));
        assert_false(manager_->has_scene("main"));
    }

    void test_scenes_queued_for_activation() {
        assert_false(manager_->scene_queued_for_activation());
        manager_->register_scene<TestScene>("main");

        assert_false(manager_->scene_queued_for_activation());
        manager_->activate("main");
        assert_true(manager_->scene_queued_for_activation());

        manager_->activate("main");
        assert_true(manager_->scene_queued_for_activation());

        window->signal_post_idle()();
        assert_false(manager_->scene_queued_for_activation());
    }

    void test_activate() {
        auto cb = [this]() {
            manager_->activate("main");
        };

        assert_raises(std::logic_error, cb);

        manager_->register_scene<TestScene>("main");

        manager_->activate("main");
        window->signal_post_idle()();

        TestScene* scr = dynamic_cast<TestScene*>(manager_->resolve_scene("main").get());
        scr->set_destroy_on_unload(false); //Don't destroy on unload

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->activate("main"); //activateing to the same place should do nothing
        window->signal_post_idle()();

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->register_scene<TestScene>("/test");

        auto initial = window->signal_post_idle().connection_count();
        manager_->activate("/test");
        assert_equal(window->signal_post_idle().connection_count(), initial + 1);
        window->signal_post_idle()();

        // Check that we disconnect the activate signal
        assert_equal(window->signal_post_idle().connection_count(), initial);

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
        window->run_frame();
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
        manager.register_scene<SceneWithArgs>("test", "arg");
    }
};


}

#endif // TEST_sceneS_H

