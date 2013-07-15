#ifndef APPLICATION_H
#define APPLICATION_H

#include <cstdint>
#include <memory>
#include <list>
#include <future>

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
    App(const unicode& title=_u("KGLT Application"), uint16_t width=640, uint16_t height=480,
            uint8_t bpp=0, bool fullscreen=false);

    //Create the window, start do_initialization in a thread, show the loading screen
    //when thread completes, hide the loading screen and run the main loop
    int32_t run();

    WindowBase& window() { return *window_; }

protected:
    Scene& scene();
    Stage& stage(StageID stage=StageID());

    void load_async(std::function<bool ()> func);

private:
    std::shared_ptr<WindowBase> window_;

    virtual bool do_init() = 0;
    virtual void do_step(double dt) = 0;
    virtual void do_cleanup() = 0;

    std::list<std::shared_future<bool> > load_tasks_;
    void check_tasks();
};

}

/**

  USAGE:

  class MyApp: public kglt::App {

  private:
    bool do_init() {
        cube_ = stage.actor(stage().geom_factory().new_cube(1.0));
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
