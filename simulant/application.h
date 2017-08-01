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

#include "keycodes.h"
#include "utils/deprecated.h"
#include "types.h"
#include "utils/unicode.h"
#include "scenes/scene_manager.h"
#include "generic/property.h"
#include "generic/data_carrier.h"
#include "scenes/scene_manager.h"

namespace smlt {

class WindowBase;
class Stage;

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
    public SceneManagerInterface {

public:
    Application(const AppConfig& config);

    //Create the window, start do_initialization in a thread, show the loading scene
    //when thread completes, hide the loading scene and run the main loop
    int32_t run();

    Property<Application, WindowBase> window = {this, &Application::window_ };
    Property<Application, generic::DataCarrier> data = { this, &Application::data_carrier_ };

    bool initialized() const { return initialized_; }

    bool has_scene(const std::string& route) const override;
    SceneBasePtr resolve_scene(const std::string& route) override;
    void activate_scene(const std::string& route) override;
    void load_scene(const std::string& route) override;
    void load_scene_in_background(const std::string& route, bool redirect_after=true) override;
    void unload_scene(const std::string& route) override;
    bool is_scene_loaded(const std::string& route) const override;
    void reset() override;
    SceneBasePtr active_scene() const override;
    void _store_scene_factory(const std::string& name, std::function<SceneBasePtr (WindowBase*)> func) override;

protected:
    StagePtr stage(StageID stage=StageID());

    bool init();

    Property<Application, AppConfig> config = { this, &Application::config_ };

private:
    std::shared_ptr<WindowBase> window_;
    std::unique_ptr<SceneManager> scene_manager_;

    bool initialized_ = false;

    virtual bool do_init() = 0;
    virtual void do_fixed_update(float dt) {}
    virtual void do_cleanup() {}

    virtual bool while_key_pressed(KeyboardCode key, double) { return false; }
    virtual bool on_key_press(KeyboardCode key) { return false; }
    virtual bool on_key_release(KeyboardCode key) { return false; }

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
