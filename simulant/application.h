/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include <cstdint>
#include <memory>
#include <list>
#include <thread>
#include <future>
#include <SDL.h>

#include "utils/deprecated.h"
#include "types.h"
#include "utils/unicode.h"
#include "screens/screen_manager.h"
#include "generic/property.h"
#include "generic/data_carrier.h"

namespace smlt {

class WindowBase;
class Stage;
class ScreenManager;

class BackgroundLoadException : public std::runtime_error {
public:
    BackgroundLoadException():
        std::runtime_error("An error occurred while running a background task") {}
};

struct AppConfig {
    unicode title = _u("Simulant Application");
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t bpp = 0;
    bool fullscreen = true;

    // Additional paths for asset loading
    std::vector<unicode> search_paths;

    // Program arguments
    std::vector<unicode> arguments;
};

class Application :
    public ScreenManagerInterface {
public:
    Application(const AppConfig& config);

    DEPRECATED
    Application(const unicode& title=_u("Simulant Application"),
        uint32_t width=1366,
        uint32_t height=768,
        uint32_t bpp=0,
        bool fullscreen=false);

    //Create the window, start do_initialization in a thread, show the loading screen
    //when thread completes, hide the loading screen and run the main loop
    int32_t run();

    Property<Application, WindowBase> window = {this, &Application::window_ };
    Property<Application, generic::DataCarrier> data = { this, &Application::data_carrier_ };

    bool initialized() const { return initialized_; }

    /* ScreenManager interface */
    virtual void register_screen(const std::string& route, ScreenFactory factory) { routes_->register_screen(route, factory); }
    virtual bool has_screen(const std::string& route) const { return routes_->has_screen(route); }
    virtual ScreenBasePtr resolve_screen(const std::string& route) { return routes_->resolve_screen(route); }
    virtual void activate_screen(const std::string& route) { routes_->activate_screen(route); }
    virtual void load_screen(const std::string& route) { routes_->load_screen(route); }
    virtual void load_screen_in_background(const std::string& route, bool redirect_after=true) { routes_->load_screen_in_background(route, redirect_after); }
    virtual void unload_screen(const std::string& route) { routes_->unload_screen(route); }
    virtual bool is_screen_loaded(const std::string& route) const { return routes_->is_screen_loaded(route); }
    virtual ScreenBasePtr active_screen() const { return routes_->active_screen(); }
    const std::unordered_map<std::string, ScreenBasePtr> routes() const override { return routes_->routes(); }
    /* End ScreenManager interface */
protected:
    StagePtr stage(StageID stage=StageID());

    bool init();

    Property<Application, AppConfig> config = { this, &Application::config_ };

private:
    std::shared_ptr<WindowBase> window_;
    std::shared_ptr<ScreenManager> routes_;

    bool initialized_ = false;

    virtual bool do_init() = 0;
    virtual void do_step(double dt) {}
    virtual void do_post_step(double dt) {}
    virtual void do_cleanup() {}

    virtual bool while_key_pressed(SDL_Keysym key, double) { return false; }
    virtual bool on_key_press(SDL_Keysym key) { return false; }
    virtual bool on_key_release(SDL_Keysym key) { return false; }

    generic::DataCarrier data_carrier_;

    AppConfig config_;
    void construct_window(const AppConfig& config);
};

}

/**

  USAGE:

  class MyApp: public smlt::App {

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
