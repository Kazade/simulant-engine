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

#ifndef SIMULANT_WINDOW_BASE_H
#define SIMULANT_WINDOW_BASE_H

#include <memory>

#include "logging.h"

#include "generic/property.h"
#include "generic/object_manager.h"
#include "generic/data_carrier.h"

#include "input/input_state.h"
#include "types.h"
#include "loader.h"
#include "event_listener.h"
#include "stats_recorder.h"
#include "screen.h"

namespace smlt {

class AssetManager;
class InputManager;

namespace ui {
    class Interface;
}

namespace scenes {
    class Loading;
}

class Application;
class InputState;
class Compositor;
class SceneImpl;
class Renderer;
class Panel;
class SoundDriver;
class StageNode;

typedef sig::signal<void (std::string, Screen*)> ScreenAddedSignal;
typedef sig::signal<void (std::string, Screen*)> ScreenRemovedSignal;

class Window :
    public Loadable,
    public RenderTarget,
    public EventListenerManager {

    DEFINE_SIGNAL(ScreenAddedSignal, signal_screen_added);
    DEFINE_SIGNAL(ScreenRemovedSignal, signal_screen_removed);

    friend class Screen;  /* Screen needs to call render_screen */
    friend class Application; /* ContextLock stuff */
public:
    typedef std::shared_ptr<Window> ptr;

    template<typename T>
    static std::shared_ptr<Window> create(Application* app) {
        auto window = std::make_shared<T>();
        window->set_application(app);
        return window;
    }

    virtual ~Window();

    virtual bool create_window(
        uint16_t width,
        uint16_t height,
        uint8_t bpp,
        bool fullscreen,
        bool enable_vsync
    );

    virtual void set_title(const std::string& title) = 0;
    virtual void cursor_position(int32_t& mouse_x, int32_t& mouse_y) = 0;
    virtual void show_cursor(bool cursor_shown=true) = 0;
    virtual void lock_cursor(bool cursor_locked=true) = 0;

    virtual void check_events() = 0;
    virtual void swap_buffers() = 0;

    uint16_t width() const override { return width_; }
    uint16_t height() const override { return height_; }
    bool is_fullscreen() const { return fullscreen_; }
    bool vsync_enabled() const { return vsync_enabled_; }

    float aspect_ratio() const;
    void set_logging_level(LogLevel level);

    void reset();

    Vec2 coordinate_from_normalized(Ratio rx, Ratio ry) {
        return Vec2(
            uint16_t(float(width()) * rx),
            uint16_t(float(height()) * ry)
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

    /* Return the number of screens connected */
    std::size_t screen_count() const;

    /* Return a specific screen given its name */
    Screen* screen(const std::string& name) const;

    void each_screen(std::function<void (std::string, Screen*)> callback);

    /* Private API for Window subclasses (public for testing)
       don't call this directly
    */
    Screen* _create_screen(
        const std::string& name,
        uint16_t width,
        uint16_t height,
        ScreenFormat format,
        uint16_t refresh_rate);

    void _destroy_screen(const std::string& name);

    /* Creates the window, but doesn't do any context initialisation */
    virtual bool _init_window() = 0;

    /* Initialises any renderer context */
    virtual bool _init_renderer(Renderer* renderer) = 0;

    bool initialize_assets_and_devices();
    void _clean_up();

    /* Audio listener stuff */

    /* Returns the current audio listener, or NULL if there
     * is no explicit audio listener set, and there are no current
     * render pipelines.
     *
     * Behaviour is:
     *
     *  - Explictly set listener
     *  - Or, First camera of first render pipeline
     *  - Or, NULL
     */
    StageNode* audio_listener();

    /* Sets a stage node explicitly as the audio listener */
    void set_audio_listener(StageNode* node);

    /* Returns true if an explicit audio listener is being used */
    bool has_explicit_audio_listener() const;

    /** Returns true if the renderer has a valid context */
    bool has_context() const { return has_context_; }

    /** Returns true if the window currently has focus */
    bool has_focus() const { return has_focus_; }

    /** Sets whether or not the window has focus, this should
     *  not be called by user code directly. */
    void set_has_focus(bool v=true) {
        has_focus_ = v;
    }

    /** Recreates the debugging panels (e.g stats) */
    void create_panels();

    /** Destroys the panels */
    void destroy_panels();
protected:
    std::shared_ptr<Renderer> renderer_;

    void set_vsync_enabled(bool vsync) {
        vsync_enabled_ = vsync;
    }

    void set_width(uint16_t width) {
        width_ = width;
    }

    void set_height(uint16_t height) {
        height_ = height;
    }

    void set_bpp(uint16_t bpp) {
        bpp_ = bpp;
    }

    void set_fullscreen(bool val) {
        fullscreen_ = val;
    }

    virtual void destroy_window() = 0;

    Window();

    void set_has_context(bool value=true);
    thread::Mutex& context_lock() { return context_lock_; }

    void set_application(Application* app) { application_ = app; }

    void update_screens(float dt);
    sig::Connection update_conn_;
public:
    void set_escape_to_quit(bool value=true) { escape_to_quit_ = value; }
    bool escape_to_quit_enabled() const { return escape_to_quit_; }

    // Panels
    void register_panel(uint8_t function_key, std::shared_ptr<Panel> panel);
    void unregister_panel(uint8_t function_key);
    void toggle_panel(uint8_t id);
    void activate_panel(uint8_t id);
    void deactivate_panel(uint8_t id);
    bool panel_is_active(uint8_t id);
private:
    Application* application_ = nullptr;

    void create_defaults();
    virtual void initialize_input_controller(InputState& controller) = 0;

    bool can_attach_sound_by_id() const { return false; }

    bool initialized_;

    uint16_t width_ = 0;
    uint16_t height_ = 0;
    uint16_t bpp_ = 0;
    bool fullscreen_ = false;
    bool vsync_enabled_ = false;

    bool escape_to_quit_ = true;

    bool has_context_ = false;
    bool has_focus_ = false;

    struct PanelEntry {
        std::shared_ptr<Panel> panel;
    };

    std::unordered_map<uint8_t, PanelEntry> panels_;

    /*
     *  Sometimes we need to destroy or recreate the GL context, if that happens while we are rendering in the
     *  main thread, then bad things happen. This lock exists so that we don't destroy the context while we are rendering.
     *  We obtain the lock before rendering, and release it after. Likewise we obtain the lock while destroying the context
     *  (we can use has_context to make sure we don't start rendering when there is no context) */
    thread::Mutex context_lock_;

    void destroy() {}

    std::shared_ptr<smlt::Compositor> compositor_;
    generic::DataCarrier data_carrier_;

    virtual std::shared_ptr<SoundDriver> create_sound_driver(const std::string& from_config) = 0;

    std::shared_ptr<InputState> input_state_;
    std::shared_ptr<InputManager> input_manager_;

    std::unordered_map<std::string, Screen::ptr> screens_;

    /* This is called by Screens to render themselves to devices. Default behaviour is a no-op */
    virtual void render_screen(Screen* screen, const uint8_t* data) {
        _S_UNUSED(screen);
        _S_UNUSED(data);
    }

    /* To be overridden by subclasses if external screens need some kind of initialization/clean_up */
    virtual bool initialize_screen(Screen* screen) {
        _S_UNUSED(screen);
        return true;
    }

    virtual void shutdown_screen(Screen* screen) {
        _S_UNUSED(screen);
    }

    StageNode* audio_listener_ = nullptr;
protected:
    InputState* _input_state() const { return input_state_.get(); }
public:
    //Read only properties
    S_DEFINE_PROPERTY(application, &Window::application_);
    S_DEFINE_PROPERTY(renderer, &Window::renderer_);
    S_DEFINE_PROPERTY(data, &Window::data_carrier_);
    S_DEFINE_PROPERTY(input, &Window::input_manager_);
    S_DEFINE_PROPERTY(input_state, &Window::input_state_);
    S_DEFINE_PROPERTY(compositor, &Window::compositor_);
};

}

#endif
