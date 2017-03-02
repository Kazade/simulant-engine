#ifndef TEST_sceneS_H
#define TEST_sceneS_H

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"
#include "global.h"

namespace {

using namespace smlt;


class SceneTests : public SimulantTestCase {
public:
    void test_load() {

    }

    void test_unload() {

    }
};

class TestScene : public Scene<TestScene> {
public:
    TestScene(WindowBase& window):
        Scene<TestScene>(window, "test_scene") {}

    void do_load() override { load_called = true; }
    void do_unload() override { unload_called = true; }
    void do_activate() override { activate_called = true; }
    void do_deactivate() override { deactivate_called = true; }

    volatile bool load_called = false;
    volatile bool unload_called = false;
    volatile bool activate_called = false;
    volatile bool deactivate_called = false;
};

class SceneManagerTests : public SimulantTestCase {
private:
    SceneManager::ptr manager_;

public:
    void set_up() {
        SimulantTestCase::set_up();
        manager_ = std::make_shared<SceneManager>(window.get());
    }

    void test_route() {
        assert_false(manager_->has_scene("/"));
        assert_false(manager_->has_scene("/test"));

        manager_->register_scene<TestScene>("/test");

        assert_true(manager_->has_scene("/test"));
        assert_false(manager_->has_scene("/"));
    }

    void test_activate_scene() {
        assert_raises(std::logic_error, std::bind(&SceneManager::activate_scene, manager_, "/"));

        manager_->register_scene<TestScene>("/");

        manager_->activate_scene("/");

        TestScene* scr = dynamic_cast<TestScene*>(manager_->resolve_scene("/").get());

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->activate_scene("/"); //activate_sceneing to the same place should do nothing

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->register_scene<TestScene>("/test");
        manager_->activate_scene("/test");

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_true(scr->deactivate_called); //Should've been deactivated
        assert_false(scr->unload_called);

        TestScene* scr2 = dynamic_cast<TestScene*>(manager_->resolve_scene("/test").get());

        assert_true(scr2->load_called);
        assert_true(scr2->activate_called);
        assert_false(scr2->deactivate_called);
        assert_false(scr2->unload_called);
    }

    void test_background_load() {
        manager_->register_scene<TestScene>("/");

        TestScene* scr = dynamic_cast<TestScene*>(manager_->resolve_scene("/").get());
        assert_false(scr->load_called);
        manager_->load_scene_in_background("/");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));        
        assert_true(scr->load_called);
    }

    void test_unload() {

    }

    void test_is_loaded() {

    }

    void test_reset() {

    }
};


}

#endif // TEST_sceneS_H

