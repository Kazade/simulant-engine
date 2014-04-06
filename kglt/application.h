#ifndef APPLICATION_H
#define APPLICATION_H

#include <cstdint>
#include <memory>
#include <list>
#include <boost/thread.hpp>

#include "types.h"
#include "kazbase/unicode.h"

namespace kglt {

class WindowBase;
class Stage;
class Scene;

class BackgroundLoadException : public std::runtime_error {
public:
    BackgroundLoadException():
        std::runtime_error("An error occurred while running a background task") {}
};

class App {
public:
    App(const unicode& title=_u("KGLT Application"),
        uint32_t width=640,
        uint32_t height=480,
        uint32_t bpp=0,
        bool fullscreen=false);

    //Create the window, start do_initialization in a thread, show the loading screen
    //when thread completes, hide the loading screen and run the main loop
    int32_t run();

    WindowBase& window() { return *window_; }

    bool initialized() const { return initialized_; }
protected:
    Scene& scene();
    StagePtr stage(StageID stage=StageID());

    void load_async(boost::function<bool ()> func);

    bool init() {
        initialized_ = do_init();
        return initialized_;
    }
private:
    std::shared_ptr<WindowBase> window_;
    bool initialized_ = false;

    virtual bool do_init() = 0;
    virtual void do_step(double dt) = 0;
    virtual void do_cleanup() = 0;

    std::list<boost::shared_future<bool> > load_tasks_;
    void check_tasks();

    virtual bool while_key_pressed(SDL_Keysym key, double) { return false; }
    virtual bool on_key_press(SDL_Keysym key) { return false; }
    virtual bool on_key_release(SDL_Keysym key) { return false; }
};

}

/**

  USAGE:

  class MyApp: public kglt::App {

  private:
    bool do_init() {
        cube_ = stage.actor(stage()->geom_factory().new_cube(1.0));
        return true;
    }

    void do_step(float dt) {
        cube_.rotate_x(10 * dt);
    }

    void do_cleanup() { }

    Actor& cube_;
  };


  MyApp my_application;
  return my_application.run();
*/

#endif // APPLICATION_H
