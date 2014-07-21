#ifndef KGLT_WINDOW_BASE_H
#define KGLT_WINDOW_BASE_H

#include <SDL.h>

#include <memory>
#include <kaztimer/kaztimer.h>

#include "resource_locator.h"

#include "idle_task_manager.h"

#include "generic/auto_weakptr.h"
#include "kazbase/logging.h"
#include "generic/manager.h"
#include "generic/data_carrier.h"
#include "resource_manager.h"
#include "types.h"
#include "viewport.h"
#include "sound.h"
#include "managers.h"

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

typedef std::function<void (double)> WindowUpdateCallback;
typedef std::shared_ptr<Loader> LoaderPtr;
typedef std::shared_ptr<LoaderType> LoaderTypePtr;

typedef generic::TemplatedManager<WindowBase, Viewport, ViewportID> ViewportManager;

class WindowBase :
    public ViewportManager,
    public Source,
    public BackgroundManager,
    public StageManager,
    public UIStageManager,
    public CameraManager,
    public ResourceManagerImpl,
    public Loadable {

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
    
    void register_loader(LoaderTypePtr loader_type);

    virtual sig::signal<void (SDL_Scancode)>& signal_key_up() = 0;
    virtual sig::signal<void (SDL_Scancode)>& signal_key_down() = 0;
    
    virtual void set_title(const std::string& title) = 0;
    virtual void cursor_position(int32_t& mouse_x, int32_t& mouse_y) = 0;
    
    virtual void check_events() = 0;
    virtual void swap_buffers() = 0;

    double delta_time() const { return delta_time_; }
    double total_time() const { return total_time_; }
    double fixed_step() const { return 1.0 / double(WindowBase::STEPS_PER_SECOND); }
    double fixed_step_interp() const;

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    
    bool run_frame();
    void update(double dt) override;

    IdleTaskManager& idle() { return idle_; }

    ViewportID new_viewport();
    ViewportPtr viewport(ViewportID viewport=ViewportID());
    void delete_viewport(ViewportID viewport);
    ViewportID default_viewport() const { return default_viewport_; }

    ResourceLocator& resource_locator() { return *resource_locator_; }
    const ResourceLocator& resource_locator() const { return *resource_locator_; }

    Keyboard& keyboard();
    Mouse& mouse();
    Joypad& joypad(uint8_t idx);
    uint8_t joypad_count() const;

    MessageBar& message_bar() { return *message_bar_; }

    void set_logging_level(LoggingLevel level);

    sig::signal<void (void)>& signal_frame_started() { return signal_frame_started_; }
    sig::signal<void (void)>& signal_frame_finished() { return signal_frame_finished_; }
    sig::signal<void (void)>& signal_pre_swap() { return signal_pre_swap_; }
    sig::signal<void (double)>& signal_step() { return signal_step_; }
    sig::signal<void (double)>& signal_post_step() { return signal_post_step_; }
    sig::signal<void (void)>& signal_shutdown() { return signal_shutdown_; }

    void stop_running() { is_running_ = false; }
    const bool is_shutting_down() const { return is_running_ == false; }

    Watcher& watcher() {
        if(!watcher_) {
            throw RuntimeError("Watcher has not been initialized");
        }
        return *watcher_;
    }

    screens::Loading& loading() { return *loading_; }

    RenderSequencePtr render_sequence();
    generic::DataCarrier& data() { return data_carrier_; }

    void enable_physics(std::shared_ptr<PhysicsEngine> engine);
    PhysicsEnginePtr physics();
    const bool has_physics_engine() const;

protected:

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

private:    
    CameraID default_ui_camera_id_;

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

    void destroy() {}

    ViewportID default_viewport_;

    ResourceLocator::ptr resource_locator_;
    std::shared_ptr<InputController> input_controller_;

    double frame_counter_time_;
    int32_t frame_counter_frames_;
    double frame_time_in_milliseconds_;

    double total_time_;

    sig::signal<void ()> signal_frame_started_;
    sig::signal<void ()> signal_pre_swap_;
    sig::signal<void ()> signal_frame_finished_;
    sig::signal<void (double)> signal_step_;
    sig::signal<void (double)> signal_post_step_;
    sig::signal<void ()> signal_shutdown_;

    std::shared_ptr<Console> console_;
    std::shared_ptr<Watcher> watcher_;

    std::shared_ptr<screens::Loading> loading_;
    sig::connection loading_update_connection_;

    std::shared_ptr<MessageBar> message_bar_;
    std::shared_ptr<kglt::RenderSequence> render_sequence_;
    generic::DataCarrier data_carrier_;

    std::shared_ptr<PhysicsEngine> physics_engine_;
};

}

#endif
