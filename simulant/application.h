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
#include <iosfwd>

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
#include "loader.h"
#include "nodes/stage_node_pool.h"


#define DEFAULT_LANGUAGE_CODE "en-us"


namespace smlt {

class Window;
class Stage;
class SharedAssetManager;
class TimeKeeper;
class StatsRecorder;
class VirtualFileSystem;
class SoundDriver;

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

    /* This is how many fixed updates should happen per second */
    uint16_t target_fixed_step_rate = 60;

    /* Whether to enable vsync or not */
    bool enable_vsync = false;

    // Additional paths for asset loading
    std::vector<Path> search_paths;

    // Program arguments
    std::vector<unicode> arguments;

    smlt::LogLevel log_level = smlt::LOG_LEVEL_INFO;

    /* If set to true, the mouse cursor will not be hidden by default */
    bool show_cursor = false;

    std::string source_language_code = DEFAULT_LANGUAGE_CODE;

    struct General {
        uint32_t stage_node_pool_size = 64;
    } general;

    struct UI {
        /** If specified, these directories are added to the path
         * temporarily while loading the default font */
        std::vector<Path> font_directories = {};

        /** The font-family that is used by default for widgets */
        std::string font_family = "Cantarell";

        /** The root font size, all Rem measurements are based on this
          * unless overridden in a UIConfig */
        uint16_t font_size = 18;
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

        /* Additional memory debug logging. If set to true information
          about scene memory usage will be logged at INFO level */
        bool additional_memory_logging = true;
    } development;
};

class Loader;
class LoaderType;

typedef std::shared_ptr<Loader> LoaderPtr;
typedef std::shared_ptr<LoaderType> LoaderTypePtr;

typedef sig::signal<void ()> FrameStartedSignal;
typedef sig::signal<void ()> FrameFinishedSignal;
typedef sig::signal<void ()> PreSwapSignal;
typedef sig::signal<void ()> PostCoroutinesSignal;

typedef sig::signal<void (float)> FixedUpdateSignal;
typedef sig::signal<void (float)> UpdateSignal;
typedef sig::signal<void (float)> LateUpdateSignal;
typedef sig::signal<void ()> PostLateUpdateSignal;
typedef sig::signal<void ()> ShutdownSignal;

class Application {
    friend class Window;

    DEFINE_SIGNAL(PreSwapSignal, signal_pre_swap);
    DEFINE_SIGNAL(PostCoroutinesSignal, signal_post_coroutines);
    DEFINE_SIGNAL(FixedUpdateSignal, signal_fixed_update);
    DEFINE_SIGNAL(UpdateSignal, signal_update);
    DEFINE_SIGNAL(LateUpdateSignal, signal_late_update);
    DEFINE_SIGNAL(PostLateUpdateSignal, signal_post_late_update);
    DEFINE_SIGNAL(ShutdownSignal, signal_shutdown);

    DEFINE_SIGNAL(FrameStartedSignal, signal_frame_started);
    DEFINE_SIGNAL(FrameFinishedSignal, signal_frame_finished);
public:
    Application(const AppConfig& config);
    virtual ~Application();

    //Create the window, start do_initialization in a thread, show the loading scene
    //when thread completes, hide the loading scene and run the main loop
    int32_t run();
    int32_t run(int argc, char* argv[]);

    bool initialized() const { return initialized_; }

    /** Returns the process ID for the application, or
     * -1 if it's unavailable or unsupported */
    ProcessID process_id() const;

    /** Returns the thread ID of the main process */
    smlt::thread::ThreadID thread_id() const;

    /** Returns an approximation of the ram usage of
     * the current process. Returns -1 if an error occurs
     * or not supported on the platform */
    int64_t ram_usage_in_bytes() const;

    /** Capacity of the global stagenode pool. This is a
     * memory buffer where all stage nodes (across all stages)
     * are stored/released */
    uint32_t stage_node_pool_capacity() const;
    uint32_t stage_node_pool_capacity_in_bytes() const;

    /** Runs a single frame of the application. You likely
     * don't want to call this! */
    bool run_frame();

    /** Runs the update and late_update methods on the active
     *  scene. You likely don't want to call this directly. */
    void run_update(float dt);

    /** Runs fixed_updates, you likely don't want to call this directly */
    void run_fixed_updates();

    /** Request the target frame time - frames will delay to
     * match it if the framerate would otherwise be too high */
    void request_frame_time(float ms);

    /* Coroutines */
    void start_coroutine(std::function<void ()> func);
    void update_coroutines();
    void stop_all_coroutines();

    /** Stops the entire application */
    void stop_running();

    /** Returns true if the application is shutting down */
    bool is_shutting_down() const;

    void shutdown();

    /* Loader things */
    LoaderPtr loader_for(const Path &filename, LoaderHint hint=LOADER_HINT_NONE);
    LoaderPtr loader_for(const std::string& loader_name, const Path& filename);
    LoaderTypePtr loader_type(const std::string& loader_name) const;

    void register_loader(LoaderTypePtr loader_type);

    /** Activates a new language for UI widgets. This will search
     *  asset paths for an .arb file for the language using fallback
     *  logic (e.g. en-gb -> en). Codes are ISO 639-1 and should be
     *  lower-case. This function will return false if the .po file
     *  cannot be found or loaded. */
    bool activate_language(const std::string& language_code);


    /**
     *  Activates a language from an ARB file that's already been loaded from
     *  disk. The language code is read from the ARB itself and no locale
     *  directories are searched. */
    bool activate_language_from_arb_data(const uint8_t* data, std::size_t byte_size);


    /**  Returns the currently active language */
    std::string active_language() const {
        return active_language_;
    }

    /** Returns the localised text for the active language. This
     *  will return source_text if no translation exists or the
     *  active language is the same as the default language in the
     *  AppConfig. This method is wrapped by the `_T()` helper macro
     *  and you should prefer to use that so that your translated strings
     *  can be detected by automated scanning. */
    unicode translated_text(const unicode& source_text) {
        auto it = active_translations_.find(source_text);
        if(it == active_translations_.end()) {
            return source_text;
        } else {
            return it->second;
        }
    }

protected:
    bool _call_init();

private:
    friend void cr_run_main(std::function<void ()> func);
    std::function<void ()> cr_synced_function_;

    void run_coroutines_and_late_update();

    thread::ThreadID main_thread_id_;
    bool has_shutdown_ = false;

    std::shared_ptr<Window> window_;
    std::shared_ptr<SceneManager> scene_manager_;
    std::shared_ptr<SharedAssetManager> asset_manager_;
    std::shared_ptr<TimeKeeper> time_keeper_;
    std::shared_ptr<StatsRecorder> stats_;
    std::shared_ptr<VirtualFileSystem> vfs_;
    std::shared_ptr<SoundDriver> sound_driver_;

    std::vector<LoaderTypePtr> loaders_;

    bool initialized_ = false;
    bool is_running_ = true;

    float frame_counter_time_ = 0.0f;
    int32_t frame_counter_frames_ = 0;
    float frame_time_in_milliseconds_ = 0.0f;

    void _call_fixed_update(float dt) {
        fixed_update(dt);
    }

    void _call_clean_up();

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
    StageNodePool* node_pool_ = nullptr;

    void construct_window(const AppConfig& config);

    ArgParser args_;

    void await_frame_time();
    uint64_t last_frame_time_us_ = 0;
    float requested_frame_time_ms_ = 0;

    std::list<cort::CoroutineID> coroutines_;
    void preload_default_font();

    std::string active_language_ = DEFAULT_LANGUAGE_CODE;
    std::map<unicode, unicode> active_translations_;
public:
    S_DEFINE_PROPERTY(window, &Application::window_);
    S_DEFINE_PROPERTY(data, &Application::data_carrier_);
    S_DEFINE_PROPERTY(scenes, &Application::scene_manager_);
    S_DEFINE_PROPERTY(args, &Application::args_);
    S_DEFINE_PROPERTY(config, &Application::config_);
    S_DEFINE_PROPERTY(stage_node_pool, &Application::node_pool_);
    S_DEFINE_PROPERTY(shared_assets, &Application::asset_manager_);
    S_DEFINE_PROPERTY(time_keeper, &Application::time_keeper_);
    S_DEFINE_PROPERTY(stats, &Application::stats_);
    S_DEFINE_PROPERTY(vfs, &Application::vfs_);
    S_DEFINE_PROPERTY(sound_driver, &Application::sound_driver_);
private:
    friend Application* get_app();
    static Application* global_app;

    mutable thread::Mutex running_lock_;

    std::vector<std::string> generate_potential_codes(const std::string& language_code);
    bool load_arb(std::shared_ptr<std::istream> stream, std::string* language_code = nullptr);
    bool load_arb_from_file(const smlt::Path& filename);
};

Application* get_app();

}

#define _T(text) \
    smlt::get_app()->translated_text((text))

#endif // APPLICATION_H
