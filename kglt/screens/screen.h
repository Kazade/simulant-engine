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

class Screen:
    public Managed<Screen> {

public:
    Screen(WindowBase& window);
    virtual ~Screen();

    void load();
    void unload();

    void activate();
    void deactivate();

    void update(double dt);

    bool is_loaded() const { return is_loaded_; }
protected:
    WindowBase& window() { return window_; }

    virtual void do_load() = 0;
    virtual void do_unload() = 0;
    virtual void do_activate() = 0;
    virtual void do_deactivate() = 0;
    virtual void do_update(double dt) {}

private:
    WindowBase& window_;
    bool is_loaded_ = false;
};



}


#endif // SCREEN_H
