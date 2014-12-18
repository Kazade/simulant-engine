#ifndef SCREEN_H
#define SCREEN_H

/**
 *  Allows you to register different screens of gameplay, and
 *  easily switch between them.
 *
 *  manager->add_route<LoadingScreen>("/");
 *  manager->add_route<MenuScreen>("/menu");
 *  manager->add_route<GameScreen>("/game");
 *
 *  manager->redirect("/");
 *  manager->background_load("/menu");
 *  if(manager->is_loaded("/menu")) {
 *      manager->redirect("/menu");
 *  }
 *  manager->unload("/");
 *  manager->redirect("/"); // Will cause loading to happen again
 *
 */


#include "../generic/managed.h"
#include <kazbase/unicode.h>
#include <kazbase/exceptions.h>

namespace kglt {

class WindowBase;

class ScreenLoadException : public RuntimeError {};

class ScreenBase {
public:
    typedef std::shared_ptr<ScreenBase> ptr;

    ScreenBase(WindowBase& window);
    virtual ~ScreenBase();

    void load();
    void unload();

    void activate();
    void deactivate();

    void step(double dt);

    bool is_loaded() const { return is_loaded_; }
protected:
    WindowBase& window() { return window_; }

    virtual void do_load() = 0;
    virtual void do_unload() {}
    virtual void do_activate() {}
    virtual void do_deactivate() {}
    virtual void do_step(double dt) {}

private:
    WindowBase& window_;
    bool is_loaded_ = false;
};

template<typename T>
class Screen : public ScreenBase, public Managed<T> {
public:
    Screen(WindowBase& window):
        ScreenBase(window) {}

    void cleanup() override {
        do_unload();
    }
};

}


#endif // SCREEN_H
