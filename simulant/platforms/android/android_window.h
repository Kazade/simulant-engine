#pragma once

#include <EGL/egl.h>
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

    void swap_buffers() override;
    void destroy_window() override;
    void check_events() override;

    void initialize_input_controller(InputState &controller) override;

    std::shared_ptr<SoundDriver> create_sound_driver(const std::string& from_config) override;

private:
    EGLDisplay dpy_ = EGL_NO_DISPLAY;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext ctx_ = EGL_NO_CONTEXT;
    EGLConfig config_ = nullptr;

    bool _init_window() override;
    bool _init_renderer(Renderer *renderer) override;

    void on_application_set(Application* app) override;

    void create_egl_surface(android_app* aapp);

    struct FingerState {
        float x = 0.0f;
        float y = 0.0f;
        float pressure = 0.0f;
    };

    std::map<int, FingerState> finger_states_;
};

}
