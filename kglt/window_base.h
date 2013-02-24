#ifndef KGLT_WINDOW_BASE_H
#define KGLT_WINDOW_BASE_H

#include <tr1/memory>

#include <sigc++/sigc++.h>
#include <kaztimer/kaztimer.h>

#include "keyboard.h"
#include "resource_locator.h"

#include "loader.h"

#include "idle_task_manager.h"

#include "kazbase/logging.h"
#include "generic/manager.h"
#include "types.h"
#include "viewport.h"

enum LoggingLevel {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4
};

namespace kglt {
    
class InputController;
class Keyboard;
class Mouse;
class Joypad;
class DebugBar;

typedef std::tr1::function<void (double)> WindowUpdateCallback;

class WindowBase :
    public generic::TemplatedManager<WindowBase, Viewport, ViewportID> {

public:    
    WindowBase();

    virtual ~WindowBase() {
        
    }
    
    Loader::ptr loader_for(const std::string& filename, const std::string& type_hint) {
        std::string final_file = resource_locator().locate_file(filename);

        //See if we can find a loader that supports this type hint
        for(LoaderType::ptr loader_type: loaders_) {
            if(loader_type->has_hint(type_hint) && loader_type->supports(filename)) {
                return loader_type->loader_for(final_file);
            }
        }

        throw DoesNotExist<Loader>("Unable to find a loader for: " + filename);
    }

    Loader::ptr loader_for(const std::string& filename) {
        std::string final_file = resource_locator().locate_file(filename);

        for(LoaderType::ptr loader_type: loaders_) {
            if(loader_type->supports(final_file) && !loader_type->requires_hint()) {
                return loader_type->loader_for(final_file);
            }
        }

        throw DoesNotExist<Loader>("Unable to find a loader for: " + filename);
    }    
    
    void register_loader(LoaderType::ptr loader_type);

    virtual sigc::signal<void, KeyCode>& signal_key_up() = 0;
    virtual sigc::signal<void, KeyCode>& signal_key_down() = 0;
    
    virtual void set_title(const std::string& title) = 0;
    virtual void cursor_position(int32_t& mouse_x, int32_t& mouse_y) = 0;
    
    virtual void check_events() = 0;
    virtual void swap_buffers() = 0;
    double delta_time() { return ktiGetDeltaTime(); }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    
    Scene& scene();
    bool update(WindowUpdateCallback step=WindowUpdateCallback());

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

    inline DebugBar& debug_bar() { assert(debug_bar_); return *debug_bar_; }

protected:
    void stop_running() { is_running_ = false; }
    
    void set_width(uint32_t width) { 
        width_ = width; 
    }
    
    void set_height(uint32_t height) {
        height_ = height; 
    }

    void init_window();

    InputController& input_controller() { assert(input_controller_); return *input_controller_; }
private:
    bool initialized_;

    std::tr1::shared_ptr<Scene> scene_;
    int32_t width_;
    int32_t height_;

    std::vector<LoaderType::ptr> loaders_;
    bool is_running_;
        
    IdleTaskManager idle_;

    KTIuint timer_;

    void destroy() {}

    ViewportID default_viewport_;

    ResourceLocator::ptr resource_locator_;
    std::tr1::shared_ptr<InputController> input_controller_;

    std::tr1::shared_ptr<DebugBar> debug_bar_;

    double frame_counter_time_;
    int32_t frame_counter_frames_;
    double frame_time_in_milliseconds_;
    KTIuint frame_timer_;
};

}

#endif
