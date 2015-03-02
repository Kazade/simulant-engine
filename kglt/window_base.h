#ifndef KGLT_WINDOW_BASE_H
#define KGLT_WINDOW_BASE_H

#include <SDL.h>

#include <memory>
#include <kaztimer/kaztimer.h>

#include "generic/property.h"
#include "generic/manager.h"
#include "generic/data_carrier.h"

#include "resource_locator.h"

#include "idle_task_manager.h"

#include "generic/auto_weakptr.h"
#include "kazbase/logging.h"
#include "resource_manager.h"
#include "types.h"
#include "sound.h"
#include "managers.h"
#include "pipeline_helper.h"
#include "screens/screen_manager.h"

namespace kglt {

namespace ui {
    class Interface;
}

namespace screens {
    class Loading;
}

class InputController;
class Keyboard;
class Mouse;
class Joypad;
class Console;
class MessageBar;
class Loader;
class LoaderType;
class RenderSequence;
class SceneImpl;
class Watcher;
class VirtualGamepad;

typedef std::function<void (double)> WindowUpdateCallback;
typedef std::shared_ptr<Loader> LoaderPtr;
typedef std::shared_ptr<LoaderType> LoaderTypePtr;

class WindowBase :
    public Source,
    public BackgroundManager,
    public StageManager,
    public UIStageManager,
    public CameraManager,
    public ResourceManagerImpl,
    public Loadable,
    public PipelineHelperAPIInterface,
    public ScreenManagerInterface,
    public RenderTarget {

public:    
    typedef std::shared_ptr<WindowBase> ptr;
    static const int STEPS_PER_SECOND = 60;

    template<typename T>
    static std::shared_ptr<WindowBase> create(int width=640, int height=480, int bpp=0, bool fullscreen=false) {
        std::shared_ptr<WindowBase> window(new T());
        if(!window->_init(width, height, bpp, fullscreen)) {
            throw InstanceInitializationError();
        }
        return window;
    }

    virtual ~WindowBase();
    
    LoaderPtr loader_for(const unicode& filename);
    LoaderPtr loader_for(const unicode& loader_name, const unicode& filename);
    
    void register_loader(LoaderTypePtr loader_type);

    virtual sig::signal<void (SDL_Scancode)>& signal_key_up() = 0;
    virtual sig::signal<void (SDL_Scancode)>& signal_key_down() = 0;
    
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

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    
    bool run_frame();
    void update(double dt) override;

    Keyboard& keyboard();
    Mouse& mouse();
    Joypad& joypad(uint8_t idx);
    uint8_t joypad_count() const;

    void set_logging_level(LoggingLevel level);

    sig::signal<void (void)>& signal_frame_started() { return signal_frame_started_; }
    sig::signal<void (void)>& signal_frame_finished() { return signal_frame_finished_; }
    sig::signal<void (void)>& signal_pre_swap() { return signal_pre_swap_; }
    sig::signal<void (double)>& signal_step() { return signal_step_; }
    sig::signal<void (double)>& signal_post_step() { return signal_post_step_; }
    sig::signal<void (void)>& signal_shutdown() { return signal_shutdown_; }

    void stop_running() { is_running_ = false; }
    const bool is_shutting_down() const { return is_running_ == false; }

    void enable_physics(std::shared_ptr<PhysicsEngine> engine);
    PhysicsEnginePtr physics();
    const bool has_physics_engine() const;

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

    virtual PipelineHelper render(StageID stage_id, CameraID camera_id) {
        return new_pipeline_helper(render_sequence_, stage_id, camera_id);
    }

    virtual PipelineHelper render(UIStageID stage_id, CameraID camera_id) {
        return new_pipeline_helper(render_sequence_, stage_id, camera_id);
    }

    virtual PipelinePtr pipeline(PipelineID pid);
    virtual bool enable_pipeline(PipelineID pid);
    virtual bool disable_pipeline(PipelineID pid);
    virtual void delete_pipeline(PipelineID pid);
    virtual bool has_pipeline(PipelineID pid) const;
    virtual bool is_pipeline_enabled(PipelineID pid) const;


    /* ScreenManager interface */
    virtual void register_screen(const unicode& route, ScreenFactory factory) { routes_->register_screen(route, factory); }
    virtual bool has_screen(const unicode& route) const { return routes_->has_screen(route); }
    virtual ScreenBasePtr resolve_screen(const unicode& route) { return routes_->resolve_screen(route); }
    virtual void activate_screen(const unicode& route) { routes_->activate_screen(route); }
    virtual void load_screen_in_background(const unicode& route, bool redirect_after=true) { routes_->load_screen_in_background(route, redirect_after); }
    virtual void unload_screen(const unicode& route) { routes_->unload_screen(route); }
    virtual bool is_screen_loaded(const unicode& route) const { return routes_->is_screen_loaded(route); }
    virtual ScreenBasePtr active_screen() const { return routes_->active_screen(); }
    /* End ScreenManager interface */

    void show_stats();
    void hide_stats();


    //Read only properties
    Property<WindowBase, Console> console = {
        [this]() -> Console& { return *this->console_.get(); }
    };

    Property<WindowBase, VirtualGamepad> virtual_joypad = {
        [this]() -> VirtualGamepad& { return *this->virtual_gamepad_.get(); }
    };

    Property<WindowBase, MessageBar> message_bar = {
        [this]() -> MessageBar& { return *this->message_bar_.get(); }
    };

    Property<WindowBase, IdleTaskManager> idle = {
        [this]() -> IdleTaskManager& { return this->idle_; }
    };

    Property<WindowBase, Watcher> watcher = {
        [this]() -> Watcher& {
            if(!watcher_) {
                throw LogicError("Watcher has not been initialized");
            } else {
                return *watcher_.get();
            }
        }
    };

    Property<WindowBase, generic::DataCarrier> data = {
        [this]() -> generic::DataCarrier& { return data_carrier_; }
    };

    Property<WindowBase, ResourceLocator> resource_locator = {
        [this]() -> ResourceLocator& { return *resource_locator_; }
    };

protected:
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


private:    
    void create_defaults();

    bool can_attach_sound_by_id() const { return false; }

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

    sig::signal<void ()> signal_frame_started_;
    sig::signal<void ()> signal_pre_swap_;
    sig::signal<void ()> signal_frame_finished_;
    sig::signal<void (double)> signal_step_;
    sig::signal<void (double)> signal_post_step_;
    sig::signal<void ()> signal_shutdown_;

    std::shared_ptr<Console> console_;
    std::shared_ptr<Watcher> watcher_;

    std::shared_ptr<screens::Loading> loading_;

    std::shared_ptr<MessageBar> message_bar_;
    std::shared_ptr<kglt::RenderSequence> render_sequence_;
    generic::DataCarrier data_carrier_;

    std::shared_ptr<PhysicsEngine> physics_engine_;

    std::shared_ptr<VirtualGamepad> virtual_gamepad_;

    std::shared_ptr<ScreenManager> routes_;
};

}

#endif
