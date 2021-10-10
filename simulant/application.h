/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include <cstdint>
#include <memory>
#include <list>

#include "arg_parser.h"
#include "keycodes.h"
#include "utils/deprecated.h"
#include "types.h"
#include "utils/unicode.h"
#include "scenes/scene_manager.h"
#include "generic/property.h"
#include "generic/data_carrier.h"
#include "scenes/scene_manager.h"
#include "screen.h"
#include "logging.h"
#include "path.h"

namespace smlt {

class Window;
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

    /* This is the frame limit; set to 0 to disable */
    uint16_t target_frame_rate = 60;

    /* Whether to enable vsync or not */
    bool enable_vsync = false;

    // Additional paths for asset loading
    std::vector<Path> search_paths;

    // Program arguments
    std::vector<unicode> arguments;

    smlt::LogLevel log_level = smlt::LOG_LEVEL_WARN;

    /* If set to true, the mouse cursor will not be hidden by default */
    bool show_cursor = false;

    struct General {
        uint32_t stage_node_pool_size = 64;
    } general;

    struct UI {
        /** If specified, these directories are added to the path
         * temporarily while loading the default font */
        std::vector<Path> font_directories = {
            "simulant/fonts/orbitron",
            "assets/simulant/fonts/orbitron"
        };

        /** The font-family that is used by default for widgets */
        std::string font_family = "Orbitron";

        /** The root font size, all Rem measurements are based on this
          * unless overridden in a UIConfig */
        uint16_t font_size = 16;
    } ui;

    struct Desktop {
        bool enable_virtual_screen = false;
        ScreenFormat virtual_screen_format = SCREEN_FORMAT_G1;
        uint16_t virtual_screen_width = 48;
        uint16_t virtual_screen_height = 32;
        uint16_t virtual_screen_integer_scale = 1;
    } desktop;

    struct Development {
        /* If set to true, profiling mode will be enabled
         * regardless of the SIMULANT_PROFILE environment variable.
         *
         * When profiling mode is enabled, the frame limit is uncapped
         * and on some platforms a profiler is enabled.
         */
#ifdef SIMULANT_PROFILE
        bool force_profiling = true;
#else
        bool force_profiling = false;
#endif
        /*
         * Set to gl1x or gl2x to force that renderer if available
            FIXME: Not yet working
        */
        std::string force_renderer = "";
        std::string force_sound_driver = "";

        /* If not empty, logging entries will be written to this
         * file as well as stdout */
        std::string log_file = "";
    } development;
};

class Application {
    friend class Window;

public:
    Application(const AppConfig& config);
    virtual ~Application() {}

    //Create the window, start do_initialization in a thread, show the loading scene
    //when thread completes, hide the loading scene and run the main loop
    int32_t run();
    int32_t run(int argc, char* argv[]);

    bool initialized() const { return initialized_; }

    /* Returns the process ID for the application, or
     * -1 if it's unavailable or unsupported */
    ProcessID process_id() const;

    /* Returns an approximation of the ram usage of
     * the current process. Returns -1 if an error occurs
     * or not supported on the platform */
    int64_t ram_usage_in_bytes() const;
protected:
    StagePtr stage(StageID stage=StageID());

    bool _call_init();

private:
    std::shared_ptr<Window> window_;
    std::shared_ptr<SceneManager> scene_manager_;

    bool initialized_ = false;


    void _call_fixed_update(float dt) {
        fixed_update(dt);
    }

    void _call_clean_up() {
        clean_up();
    }

    void _call_update(float dt) {
        update(dt);
    }

    void _call_late_update(float dt) {
        late_update(dt);
    }

    virtual bool init() = 0;
    virtual void fixed_update(float dt) {
        _S_UNUSED(dt);
    }

    virtual void update(float dt) {
        _S_UNUSED(dt);
    }

    virtual void late_update(float dt) {
        _S_UNUSED(dt);
    }

    virtual void clean_up() {}

    generic::DataCarrier data_carrier_;

    AppConfig config_;
    void construct_window(const AppConfig& config);

    ArgParser args_;

public:
    S_DEFINE_PROPERTY(window, &Application::window_);
    S_DEFINE_PROPERTY(data, &Application::data_carrier_);
    S_DEFINE_PROPERTY(scenes, &Application::scene_manager_);
    S_DEFINE_PROPERTY(args, &Application::args_);
    S_DEFINE_PROPERTY(config, &Application::config_);

private:
    friend Application* get_app();
    static Application* global_app;
};

Application* get_app();

}

#endif // APPLICATION_H
