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
        Screen<TestScreen>(window) {}

    void do_load() override { load_called = true; }
    void do_unload() override { unload_called = true; }
    void do_activate() override { activate_called = true; }
    void do_deactivate() override { deactivate_called = true; }

    bool load_called = false;
    bool unload_called = false;
    bool activate_called = false;
    bool deactivate_called = false;
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
        assert_false(manager_->has_route("/"));
        assert_false(manager_->has_route("/test"));

        manager_->add_route<TestScreen>("/test");

        assert_true(manager_->has_route("/test"));
        assert_false(manager_->has_route("/"));
    }

    void test_redirect() {
        assert_raises(ValueError, std::bind(&ScreenManager::redirect, manager_, "/"));

        manager_->add_route<TestScreen>("/");

        manager_->redirect("/");

        TestScreen* scr = dynamic_cast<TestScreen*>(manager_->resolve("/").get());

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->redirect("/"); //Redirecting to the same place should do nothing

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_false(scr->deactivate_called);
        assert_false(scr->unload_called);

        manager_->add_route<TestScreen>("/test");
        manager_->redirect("/test");

        assert_true(scr->load_called);
        assert_true(scr->activate_called);
        assert_true(scr->deactivate_called); //Should've been deactivated
        assert_false(scr->unload_called);

        TestScreen* scr2 = dynamic_cast<TestScreen*>(manager_->resolve("/test").get());

        assert_true(scr2->load_called);
        assert_true(scr2->activate_called);
        assert_false(scr2->deactivate_called);
        assert_false(scr2->unload_called);
    }

    void test_background_load() {
        manager_->add_route<TestScreen>("/");

        TestScreen* scr = dynamic_cast<TestScreen*>(manager_->resolve("/").get());
        assert_false(scr->load_called);
        manager_->background_load("/");
        assert_false(scr->load_called);

        window->idle().execute();
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

