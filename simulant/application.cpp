
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

#define DEFINE_STAGENODEPOOL
#include "nodes/stage_node_pool.h"

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

#include "asset_manager.h"
#include "application.h"
#include "scenes/scene.h"
#include "input/input_state.h"
#include "platform.h"
#include "time_keeper.h"
#include "vfs.h"
#include "compositor.h"
#include "utils/gl_error.h"
#include "nodes/ui/ui_manager.h"
#include "loaders/texture_loader.h"
#include "loaders/material_script.h"
#include "loaders/opt_loader.h"
#include "loaders/ogg_loader.h"
#include "loaders/obj_loader.h"
#include "loaders/particle_script.h"
#include "loaders/heightmap_loader.h"
#include "loaders/q2bsp_loader.h"
#include "loaders/wal_loader.h"
#include "loaders/md2_loader.h"
#include "loaders/pcx_loader.h"
#include "loaders/ttf_loader.h"
#include "loaders/fnt_loader.h"
#include "loaders/dds_texture_loader.h"
#include "loaders/wav_loader.h"
#include "loaders/ms3d_loader.h"
#include "loaders/dtex_loader.h"
#include "loaders/dcm_loader.h"
#include "utils/json.h"
#include "utils/string.h"
#include "scenes/scene_manager.h"

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
    main_thread_id_(thread::this_thread_id()),
    time_keeper_(TimeKeeper::create(1.0f / float(config.target_fixed_step_rate))),
    stats_(StatsRecorder::create()),
    vfs_(VirtualFileSystem::create()),
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

    S_INFO("Registering loaders");

    //Register the default resource loaders
    register_loader(std::make_shared<smlt::loaders::TextureLoaderType>());
    register_loader(std::make_shared<smlt::loaders::MaterialScriptLoaderType>());
    register_loader(std::make_shared<smlt::loaders::ParticleScriptLoaderType>());
    register_loader(std::make_shared<smlt::loaders::OPTLoaderType>());
    register_loader(std::make_shared<smlt::loaders::OGGLoaderType>());
    register_loader(std::make_shared<smlt::loaders::OBJLoaderType>());
    register_loader(std::make_shared<smlt::loaders::HeightmapLoaderType>());
    register_loader(std::make_shared<smlt::loaders::Q2BSPLoaderType>());
    register_loader(std::make_shared<smlt::loaders::WALLoaderType>());
    register_loader(std::make_shared<smlt::loaders::MD2LoaderType>());
    register_loader(std::make_shared<smlt::loaders::PCXLoaderType>());
    register_loader(std::make_shared<smlt::loaders::TTFLoaderType>());
    register_loader(std::make_shared<smlt::loaders::FNTLoaderType>());
    register_loader(std::make_shared<smlt::loaders::DDSTextureLoaderType>());
    register_loader(std::make_shared<smlt::loaders::WAVLoaderType>());
    register_loader(std::make_shared<smlt::loaders::MS3DLoaderType>());
    register_loader(std::make_shared<smlt::loaders::DTEXLoaderType>());
    register_loader(std::make_shared<smlt::loaders::DCMLoaderType>());
}

Application::~Application() {
    stop_running();
    shutdown();

    scene_manager_->destroy_all();

    /* This cleans up the destroyed scenes
     * before we start wiping out the node
     * pool. */
    run_coroutines_and_late_update();

    scene_manager_->clean_up();
    scene_manager_.reset();

    asset_manager_.reset();

    overlay_scene_->clean_up();
    overlay_scene_.reset();
}

void Application::preload_default_font() {
    auto& ui = config_.ui;

    FontFlags flags;
    flags.size = ui.font_size;
    auto fnt = shared_assets->new_font_from_family(ui.font_family, flags);

    if(!fnt) {
        FATAL_ERROR(ERROR_CODE_MISSING_ASSET_ERROR, "Unable to find the default font");
    }

    fnt->set_name(Font::generate_name(ui.font_family, flags.size, flags.weight, flags.style));
    fnt->set_garbage_collection_method(smlt::GARBAGE_COLLECT_NEVER);
}

std::vector<std::string> Application::generate_potential_codes(const std::string &language_code) {
    std::vector<std::string> codes;
    codes.push_back(language_code);
    auto hyphen = language_code.find("-");
    if(hyphen != std::string::npos) {
        codes.push_back(language_code.substr(0, hyphen));
    }

    return codes;
}

bool Application::construct_window(const AppConfig& config) {
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
    if(!config_copy.fullscreen && (config_copy.width == 0 || config_copy.height == 0)) {
        /* Default window size is the same as DC screen size */
        config_copy.width = 640;
        config_copy.height = 480;
    }

    if(!window_->create_window(
       config_copy.width,
       config_copy.height,
       config_copy.bpp,
       config_copy.fullscreen,
       config_copy.enable_vsync)
    ) {
        S_ERROR("[FATAL] There was an error creating the window");
        return false;
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
            request_frame_time(frame_time);
        }
#else
        float frame_time = (1.0f / float(config_copy.target_frame_rate)) * 1000.0f;
        request_frame_time(frame_time);
#endif
    }

    for(auto& search_path: config.search_paths) {
        vfs->add_search_path(search_path);
    }

    S_DEBUG("Search paths added successfully");

    if(!window_->initialize_assets_and_devices()) {
        S_ERROR("Unable to create window");
        return false;
    }

    window_->set_title(config.title.encode());

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
    return true;
}

bool Application::_call_init() {
    S_DEBUG("Initializing the application");

    if(!pre_init()) {
        S_ERROR("Application pre-initialisation failed");
        return false;
    }

    if(!construct_window(config)) {
        S_ERROR("[FATAL] Unable to create the window. Check logs. Exiting!!!");
        exit(1);
    }

    /* We can't do this in the initialiser as we need a valid
     * window before doing things like creating textures */
    asset_manager_ = SharedAssetManager::create();

    preload_default_font();

    sound_driver_ = window->create_sound_driver(config_.development.force_sound_driver);
    sound_driver_->startup();

    S_DEBUG("Sound driver started");

    class OverlayScene : public Scene {
    public:
        OverlayScene(Window* window):
            Scene(window) {}
        void load() override {}
    };

    overlay_scene_.reset(new OverlayScene(window_.get()));
    overlay_scene_->init();

    scene_manager_.reset(new SceneManager(window_.get()));
    scene_manager_->init();

    S_DEBUG("Scene manager initialized");

    initialized_ = init();

    S_DEBUG("Initialization result: {0}", initialized_);

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

void Application::request_frame_time(float ms) {
    requested_frame_time_ms_ = ms;
}

void Application::await_frame_time() {
    if(requested_frame_time_ms_ == 0) {
        return;
    }

    auto this_time = time_keeper_->now_in_us();
    while((float(this_time - last_frame_time_us_) * 0.001f) < requested_frame_time_ms_) {
        thread::sleep(0);
        this_time = time_keeper_->now_in_us();
    }
    last_frame_time_us_ = this_time;
}

void Application::run_update(float dt) {
    frame_counter_time_ += dt;
    frame_counter_frames_++;

    if(frame_counter_time_ >= 1.0f) {
        stats->set_frames_per_second(frame_counter_frames_);

        frame_time_in_milliseconds_ = 1000.0f / float(frame_counter_frames_);

        stats->set_frame_time(frame_time_in_milliseconds_);

        frame_counter_frames_ = 0;
        frame_counter_time_ = 0.0f;
    }

    if(scene_manager_) {
        scene_manager_->update(dt);
    }

    signal_update_(dt);
    _call_update(dt);
}

void Application::run_fixed_updates() {
    while(time_keeper_->use_fixed_step()) {
        float step = time_keeper_->fixed_step();

        if(scene_manager_) {
            scene_manager_->fixed_update(step);
        }

        signal_fixed_update_(step); //Trigger any steps
        _call_fixed_update(step);

        stats_->increment_fixed_steps();
    }
}

bool Application::run_frame() {
    static bool first_frame = true;

    await_frame_time(); /* Frame limiter */

    float dt = 0.0f;

    if(first_frame) {
        if(!_call_init()) {
            S_ERROR("Error while initializing, terminating application");
            return false;
        }
    } else {
        // Update timers
        time_keeper_->update();
        dt = time_keeper_->delta_time();
    }

    signal_frame_started_();

    window_->input_state->pre_update(dt);
    window_->check_events(); // Check for any window events

    auto listener = window_->audio_listener();
    if(listener) {
        sound_driver_->set_listener_properties(
            listener->absolute_position(),
            listener->absolute_rotation(),
            smlt::Vec3() // FIXME: Where do we get velocity?
        );
    }

    window_->input_state->update(dt); // Update input devices
    window_->input->update(dt); // Now update any manager stuff based on the new input state

    run_fixed_updates();
    run_update(dt);

    asset_manager_->update(time_keeper->delta_time());

    run_coroutines_and_late_update();

    asset_manager_->run_garbage_collection();

    /* Don't run the render sequence if we don't have a context, and don't update the resource
     * manager either because that probably needs a context too! */
    {
        thread::Lock<thread::Mutex> rendering_lock(window_->context_lock());
        if(window_->has_context()) {

            stats->reset_polygons_rendered();
            window_->compositor->run();

            signal_pre_swap_();

            window_->swap_buffers();
            GLChecker::end_of_frame_check();
        }
    }

    /* We totally ignore the first frame as it can take a while and messes up
     * delta time for both updates (like particle systems) and FPS
     */
    if(first_frame) {
        first_frame = false;
        time_keeper_->restart();
    } else {
        stats->increment_frames();
    }

    bool is_running = !is_shutting_down();
    if(!is_running) {
        shutdown();
    }

    signal_frame_finished_();

    return is_running;
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

    while(run_frame()) {}

    /* Make sure we unload and destroy all scenes */
    scene_manager_->destroy_all();

    /* Run any coroutines before we shut down */
    update_coroutines();

    // Shutdown and clean up the window
    window_->_clean_up();

#ifdef __DREAMCAST__
    if(PROFILING) {
        profiler_stop();
        profiler_clean_up();
    }
#endif

    if(global_app == this) {
        global_app = nullptr;
    }

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

thread::ThreadID Application::thread_id() const {
    return main_thread_id_;
}

int64_t Application::ram_usage_in_bytes() const {
    if(!is_running_) {
        return -1;
    }

    auto p = get_platform();
    if(!p) {
        return -1;
    }

    return p->process_ram_usage_in_bytes(
        process_id()
    );
}

Application* Application::global_app = nullptr;

/* Global access to the application */
Application* get_app() {
    return Application::global_app;
}

void Application::start_coroutine(std::function<void ()> func) {
    coroutines_.push_back(cort::start_coroutine(func));
}

void Application::run_coroutines_and_late_update() {
    update_coroutines();

    float dt = time_keeper_->delta_time();

    if(scene_manager_) {
        scene_manager_->late_update(dt);
    }

    signal_late_update_(dt);
    _call_late_update(dt);

    signal_post_late_update();
}

void Application::_call_clean_up() {
    clean_up();

    scene_manager_->reset();
}

void Application::stop_running() {
    thread::Lock<thread::Mutex> lock(running_lock_);
    is_running_ = false;
}

bool Application::is_shutting_down() const {
    thread::Lock<thread::Mutex> lock(running_lock_);
    return !is_running_;
}

void Application::shutdown() {
    if(has_shutdown_) {
        return;
    }

    is_running_ = false;

    signal_shutdown_();
    _call_clean_up();

    if(sound_driver_) {
        sound_driver_->shutdown();
        sound_driver_.reset();
    }

    //Shutdown the input controller
    window_->input_state_.reset();

    std::cout << "Frames rendered: " << stats->frames_run() << std::endl;
    std::cout << "Fixed updates run: " << stats->fixed_steps_run() << std::endl;
    std::cout << "Total time: " << time_keeper->total_elapsed_seconds() << std::endl;
    std::cout << "Average FPS: " << float(stats->frames_run() - 1) / (time_keeper->total_elapsed_seconds()) << std::endl;

    has_shutdown_ = true;
}

void Application::update_coroutines() {
    assert(thread_id() == thread::this_thread_id());  /* Must be called from the main thread */

    for(auto it = coroutines_.begin(); it != coroutines_.end();) {
        if(cort::resume_coroutine(*it) == cort::CO_RESULT_FINISHED) {
            cort::stop_coroutine(*it);
            it = coroutines_.erase(it);
        } else {
            /* If the coroutine yielded with a synced function
             * we run it then *resume the same coroutine* so we
             * don't increment! */
            if(cr_synced_function_) {
                cr_synced_function_();
                cr_synced_function_ = std::function<void ()>();
            } else {
                ++it;
            }
        }
    }

    signal_post_coroutines_();
}

void Application::stop_all_coroutines() {
    for(auto it = coroutines_.begin(); it != coroutines_.end();) {
        cort::stop_coroutine(*it);
        it = coroutines_.erase(it);
    }
}

void Application::register_loader(LoaderTypePtr loader) {
    if(std::find(loaders_.begin(), loaders_.end(), loader) != loaders_.end()) {
        S_WARN("Tried to add the same loader twice");
        return;
    }

    loaders_.push_back(loader);
}


std::string normalize_language_code(const std::string& language_code) {
    /* FIXME: Validate code length format etc. */

    auto underscore = language_code.find("_");
    auto hyphen = language_code.find("-");
    if(underscore != std::string::npos) {
        auto first = language_code.substr(0, underscore);
        auto second = language_code.substr(underscore + 1);
        return lower_case(first) + "-" + lower_case(second);
    } else if(hyphen != std::string::npos) {
        auto first = language_code.substr(0, hyphen);
        auto second = language_code.substr(hyphen + 1);
        return lower_case(first) + "-" + lower_case(second);
    } else {
        return lower_case(language_code);
    }
}

bool Application::load_arb_from_file(const smlt::Path& filename) {
    auto stream = vfs->open_file(filename);
    return load_arb(stream);
}

bool Application::load_arb(std::shared_ptr<std::istream> stream, std::string* language_code) {
    const char* LOCALE_KEY = "@@locale";

    auto json = json_read(stream);

    if(!json->has_key(LOCALE_KEY)) {
        S_ERROR("No {0} in the specified ARB file", LOCALE_KEY);
        return false;
    }

    std::map<std::string, unicode> id_to_translation;
    std::map<unicode, unicode> new_translations;


    for(auto& key: json->keys()) {
        if(key == "@@locale") {
            if(language_code) {
                *language_code = json[key]->to_str().value_or("");
            }
            continue;
        }

        if(starts_with(key, "@@")) {
            continue;
        }

        if(starts_with(key, "@")) {
            auto id = key.substr(1);
            if(!id_to_translation.count(id)) {
                S_ERROR("Found data at key {0} for non existent id {1}", key, id);
                continue;
            }

            auto source_text_maybe = json[key]["source_text"]->to_str();
            if(!source_text_maybe) {
                S_ERROR("Couldn't find source text for key {0}", key);
                continue;
            }

            unicode source_text(source_text_maybe.value(), "utf-8");
            source_text = source_text.replace("\\n", "\n");

            new_translations.insert(std::make_pair(source_text, id_to_translation.at(id)));
            id_to_translation.erase(id);
        } else {
            auto trans_maybe = json[key]->to_str();
            if(!trans_maybe) {
                S_ERROR("Found translation key {0} but no translation?", key);
                continue;
            }

            unicode translated_text(trans_maybe.value(), "utf-8");
            translated_text = translated_text.replace("\\n", "\n");

            id_to_translation.insert(std::make_pair(key, translated_text));
        }
   }

   active_translations_ = new_translations;

   return true;
}

bool Application::activate_language_from_arb_data(const uint8_t* data, std::size_t byte_size) {
    auto stream = std::make_shared<std::stringstream>();
    stream->write((const char*) data, byte_size);

    std::string language_code;
    auto ret = load_arb(stream, &language_code);
    if(ret) {
        active_language_ = normalize_language_code(language_code);
    }
    return ret;
}

bool Application::activate_language(const std::string &language_code) {
    auto normalized_code = normalize_language_code(language_code);

    if(active_language_ == normalized_code) {
        return true;
    }

    if(normalized_code == normalize_language_code(config_.source_language_code)) {
        active_language_ = normalized_code;
        active_translations_.clear();
        return true;
    }

    auto fallbacks = generate_potential_codes(language_code);
    for(auto& fallback: fallbacks) {
        smlt::Path filenames [] = {
            _F("{0}.arb").format(fallback),
            _F("locales/{0}.arb").format(fallback)
        };

        for(auto& filename: filenames) {
            auto path_maybe = vfs->locate_file(filename);
            if(path_maybe) {
                if(load_arb_from_file(path_maybe.value())) {
                    active_language_ = fallback;
                    return true;
                } else {
                    S_ERROR("Error loading ARB file at {0}", path_maybe.value());
                }
            }
        }
    }

    return false;
}

LoaderPtr Application::loader_for(const Path &filename, LoaderHint hint) {
    auto p = vfs->locate_file(filename);
    if(!p.has_value()) {
        S_ERROR("Couldn't get loader as file doesn't exist");
        return LoaderPtr();
    }

    Path final_file = p.value();

    std::vector<std::pair<LoaderTypePtr, LoaderPtr>> possible_loaders;

    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->supports(final_file)) {
            S_DEBUG("Found possible loader: {0}", loader_type->name());
            auto new_loader = loader_type->loader_for(final_file, vfs->open_file(final_file));
            new_loader->set_vfs(vfs_.get());

            possible_loaders.push_back(
                        std::make_pair(loader_type, new_loader)
                        );
        }
    }

    if(possible_loaders.size() == 1) {
        return possible_loaders.front().second;
    } else if(possible_loaders.size() > 1) {
        if(hint != LOADER_HINT_NONE) {
            for(auto& p: possible_loaders) {
                if(p.first->has_hint(hint)) {
                    return p.second;
                }
            }
        }

        assert(0 && "More than one possible loader was found");
        S_ERROR("More than one possible loader was found for '{0}'. Please specify a hint.", filename);
    }

    S_ERROR("No suitable loader found for {0}", filename);
    return LoaderPtr();
}


LoaderPtr Application::loader_for(const std::string& loader_name, const Path& filename) {
    auto p = vfs->locate_file(filename);
    if(!p.has_value()) {
        S_ERROR("Couldn't load file ({0}) as it doesn't exist", filename.str());
        return LoaderPtr();
    }

    Path final_file = p.value();

    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->name() == loader_name) {
            if(loader_type->supports(final_file)) {
                S_DEBUG("Found loader {0} for file: {1}", loader_name, filename.str());
                return loader_type->loader_for(final_file, vfs->open_file(final_file));
            } else {
                S_ERROR("Loader '{0}' does not support file '{1}'", loader_name, filename);
                return LoaderPtr();
            }
        }
    }

    S_ERROR("Unable to find loader for: {0}", filename.str());
    return LoaderPtr();
}

LoaderTypePtr Application::loader_type(const std::string& loader_name) const {
    for(LoaderTypePtr loader_type: loaders_) {
        if(loader_type->name() == loader_name) {
            return loader_type;
        }
    }
    return LoaderTypePtr();
}


}
