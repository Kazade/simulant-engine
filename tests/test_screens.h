#ifndef TEST_SCREENS_H
#define TEST_SCREENS_H

#include <kaztest/kaztest.h>

#include "kglt/kglt.h"
#include "global.h"

namespace {

using namespace kglt;


class ScreenTests : public KGLTTestCase {
public:
    void test_load() {

    }

    void test_unload() {

    }
};

class TestScreen : public Screen<TestScreen> {
public:
    TestScreen(WindowBase& window):
        Screen<TestScreen>(window, "test_screen") {}

    void do_load() override { load_called = true; }
    void do_unload() override { unload_called = true; }
    void do_activate() override { activate_called = true; }
    void do_deactivate() override { deactivate_called = true; }

    volatile bool load_called = false;
    volatile bool unload_called = false;
    volatile bool activate_called = false;
    volatile bool deactivate_called = false;
};

class ScreenManagerTests : public KGLTTestCase {
private:
    ScreenManager::ptr manager_;

public:
    void set_up() {
        KGLTTestCase::set_up();
        manager_ = std::make_shared<ScreenManager>(*window);
    }

    void test_route() {
        assert_false(manager_->has_screen("/"));
        assert_false(manager_->has_screen("/test"));

        manager_->register_screen("/test", screen_factory<TestScreen>());

        assert_true(manager_->has_screen("/test"));
        assert_false(manager_->has_screen("/"));
    }

    void test_activate_screen() {
        assert_raises(ValueError, std::bind(&ScreenManager::activate_screen, manager_, "/"));

        manager_->register_screen("/", screen_factory<TestScreen>());

        manager_->activate_screen("/");

        TestScreen* scr = dynamic_cast<TestScreen*>(manager_->resolve_screen("/").get());

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->activate_screen("/"); //activate_screening to the same place should do nothing

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->register_screen("/test", screen_factory<TestScreen>());
        manager_->activate_screen("/test");

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_true(scr->deactivate_called); //Should've been deactivated
        assert_false(scr->unload_called);

        TestScreen* scr2 = dynamic_cast<TestScreen*>(manager_->resolve_screen("/test").get());

        assert_true(scr2->load_called);
        assert_true(scr2->activate_called);
        assert_false(scr2->deactivate_called);
        assert_false(scr2->unload_called);
    }

    void test_background_load() {
        manager_->register_screen("/", screen_factory<TestScreen>());

        TestScreen* scr = dynamic_cast<TestScreen*>(manager_->resolve_screen("/").get());
        assert_false(scr->load_called);
        manager_->load_screen_in_background("/");
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

#endif // TEST_SCREENS_H

