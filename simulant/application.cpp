//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <chrono>
#include <future>

#ifdef _arch_dreamcast
#include "platforms/dreamcast/profiler.h"
#include "kos_window.h"
namespace smlt { typedef KOSWindow SysWindow; }
#else
#include "sdl2_window.h"
namespace smlt { typedef SDL2Window SysWindow; }
#endif

#include "application.h"
#include "scenes/loading.h"
#include "input/input_state.h"

#define SIMULANT_PROFILE_KEY "SIMULANT_PROFILE"
#define SIMULANT_SHOW_CURSOR_KEY "SIMULANT_SHOW_CURSOR"
#define SIMULANT_DEBUG_KEY "SIMULANT_DEBUG"

namespace smlt {

static bool PROFILING = false;

Application::Application(const AppConfig &config):
    config_(config) {

    args->define_arg("--help", ARG_TYPE_BOOLEAN, "display this help and exit");

    /* We're in profiling mode if we've forced it via app config
     * or the environment variable is set. If it's done by compile
     * flag the AppConfig force_profiling variable would default
     * to true */
    PROFILING = (
        config_.development.force_profiling ||
        std::getenv(SIMULANT_PROFILE_KEY) != NULL
    );

    /* Remove frame limiting in profiling mode */
    if(PROFILING) {
        config_.enable_vsync = false;
        config_.target_frame_rate = 0;
    }

    try {
        construct_window(config);
    } catch(std::runtime_error&) {
        L_ERROR("[FATAL] Unable to create the window. Check logs. Exiting!!!");
        exit(1);
    }
}

void Application::construct_window(const AppConfig& config) {
    /* Copy to remove const */
    AppConfig config_copy = config;

    /* If we're profiling, disable the frame time and vsync */
    if(PROFILING) {
        config_copy.target_frame_rate = std::numeric_limits<uint16_t>::max();
        config_copy.enable_vsync = false;
    }

    /* Allow forcing the cursor at runtime */
    if(std::getenv(SIMULANT_SHOW_CURSOR_KEY)) {
        config_copy.show_cursor = true;
    }

    // Force debug logging level
    if(std::getenv(SIMULANT_DEBUG_KEY)) {
        config_copy.log_level = LOG_LEVEL_DEBUG;
    }

    smlt::get_logger("/")->add_handler(smlt::Handler::ptr(new smlt::StdIOHandler));
    smlt::get_logger("/")->set_level(config_copy.log_level);

    L_DEBUG("Constructing the window");

    /* Fallback. If fullscreen is disabled and there is no width
     * or height, then default to 640x480 */
    if(config_copy.width == 0 && config_copy.height == 0) {
        if(!config_copy.fullscreen) {
            config_copy.width = 640;
            config_copy.height = 480;
        }
    }

    core_ = SysWindow::create(
        this,
        config_copy.width,
        config_copy.height,
        config_copy.bpp,
        config_copy.fullscreen,
        config_copy.enable_vsync
    );

    if(!core_) {
        L_ERROR("[FATAL] There was an error creating the window");
        return;
    }

    if(!config_copy.show_cursor) {
        // By default, don't show the cursor
        core_->show_cursor(false);

        // Lock the cursor by default
        core_->lock_cursor(true);
    } else {
        core_->show_cursor(true);
    }

    if(config_copy.target_frame_rate) {
        /* Don't do anything on the DC if the requested frame rate
         * is greater or equal to 60FPS
         * as the DC is capped at that anyway
         */
#ifdef _arch_dreamcast
        if(config_copy.target_frame_rate < 60) {
            float frame_time = (1.0f / float(config_copy.target_frame_rate)) * 1000.0f;
            core_->request_frame_time(frame_time);
        }
#else
        float frame_time = (1.0f / float(config_copy.target_frame_rate)) * 1000.0f;
        core_->request_frame_time(frame_time);
#endif
    }

    for(auto& search_path: config.search_paths) {
        core_->vfs->add_search_path(search_path);
    }

    L_DEBUG("Search paths added successfully");

    if(!core_->_init()) {
        throw InstanceInitializationError("Unable to create window");
    }

    core_->set_title(config.title.encode());

    /* FIXME: This is weird, the Application owns the Window, yet we're using the Window to call up to the App?
     * Not sure how to fix this without substantial changes to the frame running code */
    core_->signal_update().connect(std::bind(&Application::_call_update, this, std::placeholders::_1));
    core_->signal_late_update().connect(std::bind(&Application::_call_late_update, this, std::placeholders::_1));
    core_->signal_fixed_update().connect(std::bind(&Application::_call_fixed_update, this, std::placeholders::_1));
    core_->signal_shutdown().connect(std::bind(&Application::_call_clean_up, this));

    /* Is this a desktop window? */

#if defined(__WIN32__) || defined(__APPLE__) || defined(__LINUX__)
    SDL2Window* desktop = dynamic_cast<SDL2Window*>(core_.get());

    if(desktop) {
        if(config.desktop.enable_virtual_screen) {
            desktop->initialize_virtual_screen(
                config.desktop.virtual_screen_width,
                config.desktop.virtual_screen_height,
                config.desktop.virtual_screen_format,
                config.desktop.virtual_screen_integer_scale
            );
        }
    }
#endif

}

StagePtr Application::stage(StageID stage) {
    return core->stage(stage);
}

bool Application::_call_init() {
    L_DEBUG("Initializing the application");

    scene_manager_.reset(new SceneManager(core_.get()));

    // Add some useful scenes by default, these can be overridden in init if the
    // user so wishes
    scenes->register_scene<scenes::Loading>("_loading");
    scenes->load("_loading");

    initialized_ = init();

    // If we successfully initialized, but the user didn't specify
    // a particular scene, we just hit the root route
    if(initialized_ && !scenes->active_scene() && !scenes->scene_queued_for_activation()) {
        scenes->activate("main");
    }

    return initialized_;
}

int32_t Application::run() {
#ifdef _arch_dreamcast
    if(PROFILING) {
        profiler_init("/pc/gmon.out");
        profiler_start();
    }

    /* Try to write samples even if bad things happen */
    std::set_terminate([&]() {
        profiler_stop();
        profiler_clean_up();
    });

#endif

    if(!_call_init()) {
        L_ERROR("Error while initializing, terminating application");
        return 1;
    }

    while(core_->run_frame()) {}

    /* Make sure we unload and destroy all scenes */
    scene_manager_->destroy_all();

    // Finish running any idle tasks before we shutdown
    core_->idle->execute();

    // Shutdown and clean up the window
    core_->_clean_up();

    // Reset the scene manager before the window
    // disappears
    scene_manager_.reset();

    core_.reset();

#ifdef _arch_dreamcast
    if(PROFILING) {
        profiler_stop();
        profiler_clean_up();
    }
#endif

    return 0;
}

int32_t Application::run(int argc, char* argv[]) {
    if(!args->parse_args(argc, argv)) {
        return 2;
    }

    if(args->arg_value<bool>("help", false).value()) {
        args->print_help();
        return 0;
    }

    auto ret = Application::run();
    return ret;
}

}
