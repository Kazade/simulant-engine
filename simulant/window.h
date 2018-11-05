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

#ifndef SIMULANT_WINDOW_BASE_H
#define SIMULANT_WINDOW_BASE_H

#include <memory>

#include "deps/kazlog/kazlog.h"

#include "generic/property.h"
#include "generic/manager.h"
#include "generic/data_carrier.h"

#include "resource_locator.h"
#include "idle_task_manager.h"
#include "input/input_state.h"
#include "types.h"
#include "sound.h"
#include "managers.h"
#include "backgrounds/background_manager.h"
#include "pipeline_helper.h"
#include "scenes/scene_manager.h"
#include "loader.h"
#include "event_listener.h"
#include "time_keeper.h"
#include "stats_recorder.h"

namespace smlt {

class ResourceManager;
class InputManager;

namespace ui {
    class Interface;
}

namespace scenes {
    class Loading;
}

class Application;
class InputState;

class Loader;
class LoaderType;
class RenderSequence;
class SceneImpl;
class VirtualGamepad;
class Renderer;
class Panel;

typedef std::shared_ptr<Loader> LoaderPtr;
typedef std::shared_ptr<LoaderType> LoaderTypePtr;


typedef sig::signal<void ()> FrameStartedSignal;
typedef sig::signal<void ()> FrameFinishedSignal;
typedef sig::signal<void ()> PreSwapSignal;

typedef sig::signal<void (float)> FixedUpdateSignal;
typedef sig::signal<void (float)> UpdateSignal;
typedef sig::signal<void (float)> LateUpdateSignal;

typedef sig::signal<void ()> ShutdownSignal;

class Platform;

class Window :
    public Source,
    public StageManager,
    public BackgroundManager,
    public Loadable,
    public PipelineHelperAPIInterface,
    public RenderTarget,
    public EventListenerManager {

    DEFINE_SIGNAL(FrameStartedSignal, signal_frame_started);
    DEFINE_SIGNAL(FrameFinishedSignal, signal_frame_finished);
    DEFINE_SIGNAL(PreSwapSignal, signal_pre_swap);
    DEFINE_SIGNAL(FixedUpdateSignal, signal_fixed_update);
    DEFINE_SIGNAL(UpdateSignal, signal_update);
    DEFINE_SIGNAL(LateUpdateSignal, signal_late_update);
    DEFINE_SIGNAL(ShutdownSignal, signal_shutdown);

public:    
    typedef std::shared_ptr<Window> ptr;
    static const int STEPS_PER_SECOND = 60;


    template<typename T>
    static std::shared_ptr<Window> create(Application* app, int width, int height, int bpp, bool fullscreen, bool enable_vsync) {
        std::shared_ptr<Window> window(new T(width, height, bpp, fullscreen, enable_vsync));
        window->set_application(app);
        return window;
    }

    virtual ~Window();
    
    LoaderPtr loader_for(const unicode& filename, LoaderHint hint=LOADER_HINT_NONE);
    LoaderPtr loader_for(const unicode& loader_name, const unicode& filename);
    LoaderTypePtr loader_type(const unicode& loader_name) const;
    
    void register_loader(LoaderTypePtr loader_type);

    virtual void set_title(const std::string& title) = 0;
    virtual void cursor_position(int32_t& mouse_x, int32_t& mouse_y) = 0;
    virtual void show_cursor(bool cursor_shown=true) = 0;
    virtual void lock_cursor(bool cursor_locked=true) = 0;
    
    virtual void check_events() = 0;
    virtual void swap_buffers() = 0;

    bool is_paused() const { return is_paused_; }

    uint32_t width() const override { return width_; }
    uint32_t height() const override { return height_; }
    bool is_fullscreen() const { return fullscreen_; }
    bool vsync_enabled() const { return vsync_enabled_; }

    float aspect_ratio() const;
    
    bool run_frame();

    void _fixed_update_thunk(float dt) override;
    void _update_thunk(float dt) override;

    void set_logging_level(LoggingLevel level);

    void stop_running() { is_running_ = false; }
    bool is_shutting_down() const { return is_running_ == false; }

    void enable_virtual_joypad(VirtualGamepadConfig config, bool flipped=false);
    void disable_virtual_joypad();
    bool has_virtual_joypad() const { return bool(virtual_gamepad_); }

    void reset();

    /* PipelineHelperAPIInterface */

    virtual PipelineHelper render(StageID stage_id, CameraID camera_id) override {
        return new_pipeline_helper(render_sequence_, stage_id, camera_id);
    }

    PipelineHelper render(StagePtr stage, CameraPtr camera);

    virtual PipelinePtr pipeline(PipelineID pid) override;
    virtual bool enable_pipeline(PipelineID pid) override;
    virtual bool disable_pipeline(PipelineID pid) override;

    /* Delete a pipeline and return a nullptr so you can use the pattern
     * pipeline_ = delete_pipeline(pipeline->id()) for safety
     */
    virtual PipelinePtr delete_pipeline(PipelineID pid) override;
    virtual bool has_pipeline(PipelineID pid) const override;
    virtual bool is_pipeline_enabled(PipelineID pid) const override;

    void _cleanup();

    void each_stage(std::function<void (uint32_t, Stage*)> func);

    Vec2 coordinate_from_normalized(Ratio rx, Ratio ry) {
        return Vec2(
            uint32_t(float(width()) * rx),
            uint32_t(float(height()) * ry)
        );
    }

    void on_finger_down(
        TouchPointID touch_id,
        float normalized_x, float normalized_y, float pressure=1.0
    );

    void on_finger_up(
        TouchPointID touch_id,
        float normalized_x, float normalized_y
    );

    void on_finger_motion(
        TouchPointID touch_id,
        float normalized_x, float normalized_y,
        float dx, float dy // Between -1.0 and +1.0
    );

    void on_key_down(KeyboardCode code, ModifierKeyState modifiers);
    void on_key_up(KeyboardCode code, ModifierKeyState modifiers);


    /* Must be called directly after Window construction, it creates the window itself. The reason this
     * isn't done in create() or the constructor is that _init also sets up the default resources etc. and doesn't
     * allow a window of opportunity to manipulate the Window instance before creating the window.
     *
     * FIXME: This is dirty and hacky and should be fixed.
     */
    bool _init();

protected:    
    std::shared_ptr<Renderer> renderer_;

    RenderSequence* render_sequence();

    void set_vsync_enabled(bool vsync) {
        vsync_enabled_ = vsync;
    }

    void set_width(uint32_t width) { 
        width_ = width; 
    }
    
    void set_height(uint32_t height) {
        height_ = height; 
    }

    void set_bpp(uint32_t bpp) {
        bpp_ = bpp;
    }

    void set_fullscreen(bool val) {
        fullscreen_ = val;
    }

    virtual bool create_window() = 0;
    virtual void destroy_window() = 0;

    Window(int width, int height, int bpp, bool fullscreen, bool enable_vsync);

    void set_paused(bool value=true);
    void set_has_context(bool value=true);

    bool has_context() const { return has_context_; }
    std::mutex& context_lock() { return context_lock_; }

    void set_application(Application* app) { application_ = app; }

    void set_escape_to_quit(bool value=true) { escape_to_quit_ = value; }
    bool escape_to_quit_enabled() const { return escape_to_quit_; }
public:
    // Panels
    void register_panel(uint8_t function_key, std::shared_ptr<Panel> panel);
    void unregister_panel(uint8_t function_key);

private:    
    Application* application_ = nullptr;

    void create_defaults();
    virtual void initialize_input_controller(InputState& controller) = 0;

    bool can_attach_sound_by_id() const { return false; }

    std::shared_ptr<ResourceManager> resource_manager_;
    bool initialized_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t bpp_ = 0;
    bool fullscreen_ = false;
    bool vsync_enabled_ = false;

    bool escape_to_quit_ = true;

    std::vector<LoaderTypePtr> loaders_;
    bool is_running_;
        
    IdleTaskManager idle_;

    bool is_paused_ = false;
    bool has_context_ = false;


    struct PanelEntry {
        std::shared_ptr<Panel> panel;
    };

    std::unordered_map<uint8_t, PanelEntry> panels_;

    /*
     *  Sometimes we need to destroy or recreate the GL context, if that happens while we are rendering in the
     *  main thread, then bad things happen. This lock exists so that we don't destroy the context while we are rendering.
     *  We obtain the lock before rendering, and release it after. Likewise we obtain the lock while destroying the context
     *  (we can use has_context to make sure we don't start rendering when there is no context) */
    std::mutex context_lock_;

    void destroy() {}

    ResourceLocator::ptr resource_locator_;

    float frame_counter_time_;
    int32_t frame_counter_frames_;
    float frame_time_in_milliseconds_;

    std::shared_ptr<scenes::Loading> loading_;
    std::shared_ptr<smlt::RenderSequence> render_sequence_;
    generic::DataCarrier data_carrier_;

    std::shared_ptr<VirtualGamepad> virtual_gamepad_;
    std::unique_ptr<BackgroundManager> background_manager_;
    std::shared_ptr<TimeKeeper> time_keeper_;

    StatsRecorder stats_;

    std::shared_ptr<SoundDriver> sound_driver_;

    virtual std::shared_ptr<SoundDriver> create_sound_driver() = 0;

    std::shared_ptr<InputState> input_state_;
    std::shared_ptr<InputManager> input_manager_;

    void await_frame_time();
    uint64_t last_frame_time_us_ = 0;
    float requested_frame_time_ms_ = 0;
protected:
    InputState* _input_state() const { return input_state_.get(); }

    std::shared_ptr<Platform> platform_;
public:
    //Read only properties
    Property<Window, ResourceManager> shared_assets = { this, &Window::resource_manager_ };
    Property<Window, Application> application = { this, &Window::application_ };
    Property<Window, VirtualGamepad> virtual_joypad = { this, &Window::virtual_gamepad_ };
    Property<Window, Renderer> renderer = { this, &Window::renderer_ };
    Property<Window, TimeKeeper> time_keeper = { this, &Window::time_keeper_ };

    Property<Window, IdleTaskManager> idle = { this, &Window::idle_ };
    Property<Window, generic::DataCarrier> data = { this, &Window::data_carrier_ };
    Property<Window, ResourceLocator> resource_locator = { this, &Window::resource_locator_ };

    Property<Window, InputManager> input = {this, &Window::input_manager_};
    Property<Window, InputState> input_state = {this, &Window::input_state_};
    Property<Window, StatsRecorder> stats = { this, &Window::stats_ };
    Property<Window, Platform> platform = {this, &Window::platform_};

    SoundDriver* _sound_driver() const { return sound_driver_.get(); }

    void run_update();
    void run_fixed_updates();
    void request_frame_time(float ms);
};

}

#endif
