
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
#include <cstdlib>

#ifdef __DREAMCAST__
#include "platforms/dreamcast/profiler.h"
#include "kos_window.h"
namespace smlt { typedef KOSWindow SysWindow; }
#elif defined(__PSP__)
#include <pspkernel.h>
#include "platforms/psp/psp_window.h"
namespace smlt { typedef PSPWindow SysWindow; }
#else
#include "sdl2_window.h"
namespace smlt { typedef SDL2Window SysWindow; }
#endif

#ifdef __linux__
#include <sys/types.h>
#include <unistd.h>
#elif defined(__WIN32__)
#include <processthreadsapi.h>
#endif

#include "application.h"
#include "scenes/loading.h"
#include "input/input_state.h"
#include "platform.h"

#define SIMULANT_PROFILE_KEY "SIMULANT_PROFILE"
#define SIMULANT_SHOW_CURSOR_KEY "SIMULANT_SHOW_CURSOR"
#define SIMULANT_DEBUG_KEY "SIMULANT_DEBUG"

extern "C" {

void *__stack_chk_guard = (void *)0x69420A55;

__attribute__((weak,noreturn)) void __stack_chk_fail(void) {

#ifdef __DREAMCAST__
    /* Reset video mode, clear screen */
    vid_set_mode(DM_640x480, PM_RGB565);
    vid_empty();

    unsigned int pr = arch_get_ret_addr();
    fprintf(stderr, "Stack overflow detected! (%x)", pr);
#else
    fprintf(stderr, "Stack overflow detected!");
#endif
    abort();
}

}


namespace smlt {

static bool PROFILING = false;

Application::Application(const AppConfig &config):
    config_(config) {

    args->define_arg("--help", ARG_TYPE_BOOLEAN, "display this help and exit");

    /* Set the global app instance
     * We do this twice, here and at the start of run(), just in case
     * someone constructs two, but only calls run on one or something. */
    global_app = this;

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
        S_ERROR("[FATAL] Unable to create the window. Check logs. Exiting!!!");
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

    if(!config_copy.development.log_file.empty()) {
        smlt::get_logger("/")->add_handler(
            smlt::Handler::ptr(new smlt::FileHandler(config_copy.development.log_file))
        );
    }

    S_DEBUG("Constructing the window");

    window_ = SysWindow::create(this);

    /* Fallback. If fullscreen is disabled and there is no width
     * or height, then default to 640x480 */
    if(config_copy.width == 0 || config_copy.height == 0) {
        if(config_copy.fullscreen) {
            /* Use the native resolution */
            Resolution native = get_platform()->native_resolution();
            config_copy.width = native.width;
            config_copy.height = native.height;
        } else {
            /* Default window size is the same as DC screen size */
            config_copy.width = 640;
            config_copy.height = 480;
        }
    }

    if(!window_->create_window(
       config_copy.width,
       config_copy.height,
       config_copy.bpp,
       config_copy.fullscreen,
       config_copy.enable_vsync)
    ) {
        S_ERROR("[FATAL] There was an error creating the window");
        return;
    }

    if(!config_copy.show_cursor) {
        // By default, don't show the cursor
        window_->show_cursor(false);

        // Lock the cursor by default
        window_->lock_cursor(true);
    } else {
        window_->show_cursor(true);
    }

    if(config_copy.target_frame_rate) {
        /* Don't do anything on the DC if the requested frame rate
         * is greater or equal to 60FPS
         * as the DC is capped at that anyway
         */
#ifdef __DREAMCAST__
        if(config_copy.target_frame_rate < 60) {
            float frame_time = (1.0f / float(config_copy.target_frame_rate)) * 1000.0f;
            window_->request_frame_time(frame_time);
        }
#else
        float frame_time = (1.0f / float(config_copy.target_frame_rate)) * 1000.0f;
        window_->request_frame_time(frame_time);
#endif
    }

    for(auto& search_path: config.search_paths) {
        window_->vfs->add_search_path(search_path);
    }

    S_DEBUG("Search paths added successfully");

    if(!window_->initialize_assets_and_devices()) {
        throw InstanceInitializationError("Unable to create window");
    }

    window_->set_title(config.title.encode());

    /* FIXME: This is weird, the Application owns the Window, yet we're using the Window to call up to the App?
     * Not sure how to fix this without substantial changes to the frame running code */
    window_->signal_update().connect(std::bind(&Application::_call_update, this, std::placeholders::_1));
    window_->signal_late_update().connect(std::bind(&Application::_call_late_update, this, std::placeholders::_1));
    window_->signal_fixed_update().connect(std::bind(&Application::_call_fixed_update, this, std::placeholders::_1));
    window_->signal_shutdown().connect(std::bind(&Application::_call_clean_up, this));

    /* Is this a desktop window? */

#if defined(__WIN32__) || defined(__APPLE__) || defined(__LINUX__)
    SDL2Window* desktop = dynamic_cast<SDL2Window*>(window_.get());

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
    return window->stage(stage);
}

bool Application::_call_init() {
    S_DEBUG("Initializing the application");

    scene_manager_.reset(new SceneManager(window_.get()));

    // Add some useful scenes by default, these can be overridden in init if the
    // user so wishes
    scenes->register_scene<scenes::Loading>("_loading");

    initialized_ = init();

    // If we successfully initialized, but the user didn't specify
    // a particular scene, we just hit the root route
    if(initialized_ && !scenes->active_scene() && !scenes->scene_queued_for_activation()) {
        scenes->activate("main");
    }

    return initialized_;
}

static void on_terminate() {
#ifdef __DREAMCAST__
    if(PROFILING) {
        profiler_stop();
        profiler_clean_up();
    }
#endif

#ifdef __PSP__
    /* Try to notify and exit cleanly on PSP */
    sceKernelExitGame();
#endif
}

int32_t Application::run() {
#ifdef __DREAMCAST__
    if(PROFILING) {
        profiler_init("/pc/gmon.out");
        profiler_start();
    }
#endif


    /* Try to write samples even if bad things happen */
    std::set_terminate(on_terminate);

    if(!_call_init()) {
        S_ERROR("Error while initializing, terminating application");
        return 1;
    }

    while(window_->run_frame()) {}

    /* Make sure we unload and destroy all scenes */
    scene_manager_->destroy_all();

    // Finish running any idle tasks before we shutdown
    window_->idle->execute();

    // Shutdown and clean up the window
    window_->_clean_up();

    // Reset the scene manager before the window
    // disappears
    scene_manager_.reset();

    window_.reset();

#ifdef __DREAMCAST__
    if(PROFILING) {
        profiler_stop();
        profiler_clean_up();
    }
#endif

    return 0;
}

int32_t Application::run(int argc, char* argv[]) {
    /* Set the global app instance */
    global_app = this;

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

ProcessID Application::process_id() const {
#ifdef __linux__
    return getpid();
#elif defined(__WIN32__)
    return GetCurrentProcessId();
#else
    return -1;
#endif
}

int64_t Application::ram_usage_in_bytes() const {
    return get_platform()->process_ram_usage_in_bytes(
        process_id()
    );
}

Application* Application::global_app = nullptr;

/* Global access to the application */
Application* get_app() {
    return Application::global_app;
}

}
