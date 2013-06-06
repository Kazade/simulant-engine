#ifndef KGLT_WINDOW_BASE_H
#define KGLT_WINDOW_BASE_H

#include <tr1/memory>

#include <sigc++/sigc++.h>
#include <kaztimer/kaztimer.h>

#include "keyboard.h"
#include "resource_locator.h"

#include "idle_task_manager.h"

#include "kazbase/logging.h"
#include "generic/manager.h"

#include "types.h"
#include "viewport.h"
#include "sound.h"

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

class Loader;
class LoaderType;

class Watcher;

typedef std::tr1::function<void (double)> WindowUpdateCallback;
typedef std::shared_ptr<Loader> LoaderPtr;
typedef std::shared_ptr<LoaderType> LoaderTypePtr;

class WindowBase :
    public generic::TemplatedManager<WindowBase, Viewport, ViewportID>,
    public Source {

public:    
    typedef std::shared_ptr<WindowBase> ptr;

    template<typename T>
    static std::shared_ptr<WindowBase> create(int width=640, int height=480, int bpp=0, bool fullscreen=false) {
        std::shared_ptr<WindowBase> window(new T());
        if(!window->init(width, height, bpp, fullscreen)) {
            throw InstanceInitializationError();
        }
        return window;
    }

    virtual ~WindowBase();
    
    LoaderPtr loader_for(const unicode& filename);
    
    void register_loader(LoaderTypePtr loader_type);

    virtual sigc::signal<void, KeyCode>& signal_key_up() = 0;
    virtual sigc::signal<void, KeyCode>& signal_key_down() = 0;
    
    virtual void set_title(const std::string& title) = 0;
    virtual void cursor_position(int32_t& mouse_x, int32_t& mouse_y) = 0;
    
    virtual void check_events() = 0;
    virtual void swap_buffers() = 0;

    double delta_time() const { return delta_time_; }
    double total_time() const { return total_time_; }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    
    Scene& scene();
    const Scene& scene() const;

    bool update();

    IdleTaskManager& idle() { return idle_; }

    ViewportID new_viewport();
    Viewport& viewport(ViewportID viewport=ViewportID());
    void delete_viewport(ViewportID viewport);
    ViewportID default_viewport() const { return default_viewport_; }

    ResourceLocator& resource_locator() { return *resource_locator_; }

    Keyboard& keyboard();
    Mouse& mouse();
    Joypad& joypad(uint8_t idx);
    uint8_t joypad_count() const;

    void set_logging_level(LoggingLevel level);

    sigc::signal<void>& signal_frame_started() { return signal_frame_started_; }
    sigc::signal<void>& signal_frame_finished() { return signal_frame_finished_; }
    sigc::signal<void>& signal_pre_swap() { return signal_pre_swap_; }
    sigc::signal<void, double>& signal_step() { return signal_step_; }
    sigc::signal<void>& signal_shutdown() { return signal_shutdown_; }

    ui::Interface& ui() { return *interface_; }
    void load_ui(const std::string& rml_file);

    void stop_running() { is_running_ = false; }

    Watcher& watcher() {
        if(!watcher_) {
            throw RuntimeError("Watcher has not been initialized");
        }
        return *watcher_;
    }

    screens::Loading& loading() { return *loading_; }

protected:

    void set_width(uint32_t width) { 
        width_ = width; 
    }
    
    void set_height(uint32_t height) {
        height_ = height; 
    }

    bool init(int width, int height, int bpp, bool fullscreen);
    virtual bool create_window(int width, int height, int bpp, bool fullscreen) = 0;

    InputController& input_controller() { assert(input_controller_); return *input_controller_; }

    WindowBase();

private:    
    bool can_attach_sound_by_id() const { return false; }

    bool initialized_;

    std::shared_ptr<Scene> scene_;
    int32_t width_;
    int32_t height_;

    std::vector<LoaderTypePtr> loaders_;
    bool is_running_;
        
    IdleTaskManager idle_;

    KTIuint fixed_timer_;
    KTIuint variable_timer_;
    double delta_time_;

    void destroy() {}

    ViewportID default_viewport_;

    ResourceLocator::ptr resource_locator_;
    std::shared_ptr<InputController> input_controller_;

    double frame_counter_time_;
    int32_t frame_counter_frames_;
    double frame_time_in_milliseconds_;

    double total_time_;

    sigc::signal<void> signal_frame_started_;
    sigc::signal<void> signal_pre_swap_;
    sigc::signal<void> signal_frame_finished_;
    sigc::signal<void, double> signal_step_;
    sigc::signal<void> signal_shutdown_;

    std::shared_ptr<ui::Interface> interface_;
    std::shared_ptr<Console> console_;
    std::shared_ptr<Watcher> watcher_;

    std::shared_ptr<screens::Loading> loading_;
};

}

#endif
