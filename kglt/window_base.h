#ifndef KGLT_WINDOW_BASE_H
#define KGLT_WINDOW_BASE_H

#include <SDL.h>

#include <memory>
#include "kaztimer/kaztimer.h"

#include "generic/property.h"
#include "generic/manager.h"
#include "generic/data_carrier.h"

#include "resource_locator.h"
#include "idle_task_manager.h"
#include "input_controller.h"
#include "generic/auto_weakptr.h"
#include "kazbase/logging.h"
#include "types.h"
#include "sound.h"
#include "managers.h"
#include "pipeline_helper.h"
#include "screens/screen_manager.h"
#include "loader.h"


namespace kglt {

class ResourceManager;

namespace ui {
    class Interface;
}

namespace screens {
    class Loading;
}

class Application;
class InputController;
class Keyboard;
class Mouse;
class Joypad;
class MessageBar;
class Loader;
class LoaderType;
class RenderSequence;
class SceneImpl;
class Watcher;
class VirtualGamepad;
class Renderer;
class Panel;

typedef std::function<void (double)> WindowUpdateCallback;
typedef std::shared_ptr<Loader> LoaderPtr;
typedef std::shared_ptr<LoaderType> LoaderTypePtr;

class Stats {
public:
    uint32_t subactors_rendered() const { return subactors_renderered_; }
    void set_subactors_rendered(uint32_t value) {
        subactors_renderered_ = value;
    }

    uint32_t frames_per_second() const { return frames_per_second_; }
    void set_frames_per_second(uint32_t value) {
        frames_per_second_ = value;
    }
private:
    uint32_t subactors_renderered_;
    uint32_t frames_per_second_;
};

typedef sig::signal<void ()> FrameStartedSignal;
typedef sig::signal<void ()> FrameFinishedSignal;
typedef sig::signal<void ()> PreSwapSignal;
typedef sig::signal<void (double)> StepSignal;
typedef sig::signal<void (double)> PostStepSignal;
typedef sig::signal<void ()> ShutdownSignal;
typedef sig::signal<void (SDL_Scancode)> KeyUpSignal;
typedef sig::signal<void (SDL_Scancode)> KeyDownSignal;

class WindowBase :
    public Source,
    public StageManager,
    public UIStageManager,
    public CameraManager,
    public Loadable,
    public PipelineHelperAPIInterface,
    public RenderTarget {

    DEFINE_SIGNAL(FrameStartedSignal, signal_frame_started);
    DEFINE_SIGNAL(FrameFinishedSignal, signal_frame_finished);
    DEFINE_SIGNAL(PreSwapSignal, signal_pre_swap);
    DEFINE_SIGNAL(StepSignal, signal_step);
    DEFINE_SIGNAL(PostStepSignal, signal_post_step);
    DEFINE_SIGNAL(ShutdownSignal, signal_shutdown);

public:    
    typedef std::shared_ptr<WindowBase> ptr;
    static const int STEPS_PER_SECOND = 60;

    template<typename T>
    static std::shared_ptr<WindowBase> create(Application* app, int width=640, int height=480, int bpp=0, bool fullscreen=false) {
        std::shared_ptr<WindowBase> window(new T());
        window->set_application(app);

        if(!window->_init(width, height, bpp, fullscreen)) {
            throw InstanceInitializationError();
        }
        return window;
    }

    virtual ~WindowBase();
    
    LoaderPtr loader_for(const unicode& filename, LoaderHint hint=LOADER_HINT_NONE);
    LoaderPtr loader_for(const unicode& loader_name, const unicode& filename);
    LoaderTypePtr loader_type(const unicode& loader_name) const;
    
    void register_loader(LoaderTypePtr loader_type);

    virtual KeyUpSignal& signal_key_up() = 0;
    virtual KeyDownSignal& signal_key_down() = 0;
    
    virtual void set_title(const std::string& title) = 0;
    virtual void cursor_position(int32_t& mouse_x, int32_t& mouse_y) = 0;
    virtual void show_cursor(bool cursor_shown=true) = 0;
    
    virtual void check_events() = 0;
    virtual void swap_buffers() = 0;

    double delta_time() const { return delta_time_; }
    double total_time() const { return total_time_; }
    double fixed_step() const { return 1.0 / double(WindowBase::STEPS_PER_SECOND); }
    double fixed_step_interp() const;
    bool is_paused() const { return is_paused_; }

    uint32_t width() const override { return width_; }
    uint32_t height() const override { return height_; }
    float aspect_ratio() const { return float(width_) / float(height_); }
    
    bool run_frame();
    void update(double dt) override;

    Mouse& mouse();
    Joypad& joypad(uint8_t idx);
    uint8_t joypad_count() const;

    void set_logging_level(LoggingLevel level);

    void stop_running() { is_running_ = false; }
    const bool is_shutting_down() const { return is_running_ == false; }

    void enable_virtual_joypad(VirtualDPadDirections directions, int button_count, bool flipped=false);
    void disable_virtual_joypad();
    bool has_virtual_joypad() const { return bool(virtual_gamepad_); }

    void reset();

    // Only public for testing purposes. DO NOT USE!
    void handle_mouse_motion(int x, int y, bool pos_normalized=false);
    void handle_mouse_button_down(int button);
    void handle_mouse_button_up(int button);

    void handle_touch_down(int finger_id, int x, int y);
    void handle_touch_motion(int finger_id, int x, int y);
    void handle_touch_up(int finger_id, int x, int y);

    /* PipelineHelperAPIInterface */

    virtual PipelineHelper render(StageID stage_id, CameraID camera_id) override {
        return new_pipeline_helper(render_sequence_, stage_id, camera_id);
    }

    virtual PipelineHelper render(UIStageID stage_id, CameraID camera_id) override {
        return new_pipeline_helper(render_sequence_, stage_id, camera_id);
    }

    virtual PipelinePtr pipeline(PipelineID pid) override;
    virtual bool enable_pipeline(PipelineID pid) override;
    virtual bool disable_pipeline(PipelineID pid) override;
    virtual void delete_pipeline(PipelineID pid) override;
    virtual bool has_pipeline(PipelineID pid) const override;
    virtual bool is_pipeline_enabled(PipelineID pid) const override;

protected:    
    std::shared_ptr<Renderer> renderer_;

    RenderSequencePtr render_sequence();

    void set_width(uint32_t width) { 
        width_ = width; 
    }
    
    void set_height(uint32_t height) {
        height_ = height; 
    }

    bool _init(int width, int height, int bpp, bool fullscreen);
    virtual bool create_window(int width, int height, int bpp, bool fullscreen) = 0;

    InputController& input_controller() { assert(input_controller_); return *input_controller_; }

    WindowBase();

    void set_paused(bool value=true);
    void set_has_context(bool value=true);

    bool has_context() const { return has_context_; }
    std::mutex& context_lock() { return context_lock_; }

    void set_application(Application* app) { application_ = app; }
public:
    // Background things
    BackgroundID new_background() { return background_manager_->new_background(); }
    BackgroundID new_background_from_file(const unicode& filename, float scroll_x=0.0, float scroll_y=0.0) {
        return background_manager_->new_background_from_file(filename, scroll_x, scroll_y);
    }

    BackgroundPtr background(BackgroundID bid) {
        return background_manager_->background(bid);
    }

    bool has_background(BackgroundID bid) const {
        return background_manager_->has_background(bid);
    }

    void delete_background(BackgroundID bid) {
        background_manager_->delete_background(bid);
    }

    uint32_t background_count() const { return background_manager_->background_count(); }

    // Panels
    void register_panel(uint8_t function_key, std::shared_ptr<Panel> panel);
    void unregister_panel(uint8_t function_key);

private:    
    Application* application_ = nullptr;

    void create_defaults();

    bool can_attach_sound_by_id() const { return false; }

    ResourceManager* resource_manager_;
    bool initialized_;

    int32_t width_;
    int32_t height_;

    std::vector<LoaderTypePtr> loaders_;
    bool is_running_;
        
    IdleTaskManager idle_;

    KTIuint fixed_timer_;
    KTIuint variable_timer_;
    double delta_time_;
    double fixed_step_interp_ = 0.0;
    bool is_paused_ = false;
    bool has_context_ = false;


    struct PanelEntry {
        std::shared_ptr<Panel> panel;
        InputConnection keyboard_connection;
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
    std::shared_ptr<InputController> input_controller_;

    double frame_counter_time_;
    int32_t frame_counter_frames_;
    double frame_time_in_milliseconds_;

    double total_time_ = 0.0;

    std::shared_ptr<Watcher> watcher_;
    std::shared_ptr<screens::Loading> loading_;
    std::shared_ptr<MessageBar> message_bar_;
    std::shared_ptr<kglt::RenderSequence> render_sequence_;
    generic::DataCarrier data_carrier_;

    std::shared_ptr<VirtualGamepad> virtual_gamepad_;
    std::unique_ptr<BackgroundManager> background_manager_;

    Stats stats_;

public:

    //Read only properties
    Property<WindowBase, ResourceManager> shared_assets = { this, &WindowBase::resource_manager_ };
    Property<WindowBase, Application> application = { this, &WindowBase::application_ };
    Property<WindowBase, VirtualGamepad> virtual_joypad = { this, &WindowBase::virtual_gamepad_ };
    Property<WindowBase, MessageBar> message_bar = { this, &WindowBase::message_bar_ };
    Property<WindowBase, Renderer> renderer = { this, &WindowBase::renderer_ };

    Property<WindowBase, Watcher> watcher = {
        this, [](const WindowBase* self) -> Watcher* {
            if(!self->watcher_) {
                throw LogicError("Watcher has not been initialized");
            } else {
                return self->watcher_.get();
            }
        }
    };

    Property<WindowBase, IdleTaskManager> idle = { this, &WindowBase::idle_ };
    Property<WindowBase, generic::DataCarrier> data = { this, &WindowBase::data_carrier_ };
    Property<WindowBase, ResourceLocator> resource_locator = { this, &WindowBase::resource_locator_ };

    Property<WindowBase, Keyboard> keyboard = {
        this, [](WindowBase* self) -> Keyboard* {
            return &self->input_controller_->keyboard();
        }
    };

    Property<WindowBase, Stats> stats = { this, &WindowBase::stats_ };
};

}

#endif
