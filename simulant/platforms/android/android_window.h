#pragma once

#include <EGL/egl.h>
#include <jni.h>
#include "../../window.h"

struct android_app;

namespace smlt {

class AndroidWindow : public Window {
public:
    static Window::ptr create(Application* app) {
        return Window::create<AndroidWindow>(app);
    }

    AndroidWindow();

    void set_title(const std::string&) override {} // No-op
    void cursor_position(int32_t&, int32_t&) override {} // No-op
    void show_cursor(bool) override {} // No-op
    void lock_cursor(bool) override {} // No-op

    void destroy_window() override;
    void check_events() override;

    void initialize_input_controller(InputState &controller) override;

    std::shared_ptr<SoundDriver> create_sound_driver(const std::string& from_config) override;

    bool can_render() const override { return surface_ != EGL_NO_SURFACE; }

    /* Vibrates the phone (or whatever device), with the system's vibrator */
    bool has_haptics() const override;
    void start_haptics(NormalizedFloat strength, const Seconds& duration) override;
    void stop_haptics() override;

    /* Called as Android creates and destroys the native window. These have
     * to be dealt with straight away, see android_handle_command */
    void on_native_window_created(android_app* aapp);
    void on_native_window_destroyed();

private:
    void do_swap_buffers() override;

    EGLDisplay dpy_ = EGL_NO_DISPLAY;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext ctx_ = EGL_NO_CONTEXT;
    EGLConfig config_ = nullptr;

    /* Whether the context can be current without a surface, so GL calls made
     * while there's no window (e.g. texture uploads) still work */
    bool surfaceless_ = false;

    bool _init_window() override;
    bool _init_renderer(Renderer *renderer) override;

    void on_application_set(Application* app) override;

    void create_egl_surface(android_app* aapp);

    /* Lifts every finger that's down, e.g. when the app loses focus and
     * won't hear about them being lifted */
    void release_all_fingers();

    /* Reads the safe area from the window's insets */
    void update_safe_area();

    /* The system's vibrator, looked up the first time it's needed (and kept
     * as a global reference) */
    void find_vibrator() const;
    mutable bool vibrator_checked_ = false;
    mutable jobject vibrator_ = nullptr;
    mutable bool has_amplitude_control_ = false;

    struct FingerState {
        float x = 0.0f;
        float y = 0.0f;
        float pressure = 0.0f;
        bool down = false;
    };

    std::map<int, FingerState> finger_states_;
};

}
