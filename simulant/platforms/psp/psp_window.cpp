
#include "psp_window.h"
#include "../../sound_drivers/null_sound_driver.h"

#include "../../renderers/renderer_config.h"

namespace smlt {

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define SCREEN_DEPTH 32

void PSPWindow::swap_buffers() {
    eglSwapBuffers(dpy_, surface_);
}

static const EGLint attrib_list [] = {
    EGL_RED_SIZE, 5,
    EGL_GREEN_SIZE, 6,
    EGL_BLUE_SIZE, 5,
    EGL_ALPHA_SIZE, 0,
    EGL_DEPTH_SIZE, 0,
    EGL_NONE
};

bool PSPWindow::_init_window() {
    dpy_ = eglGetDisplay(0);
    eglInitialize(dpy_, NULL, NULL);

    EGLConfig config;
    EGLint num_configs;

    eglChooseConfig(dpy_, attrib_list, &config, 1, &num_configs);

    if(!num_configs) {
        return false;
    }

    EGLint width, height;

    eglGetConfigAttrib(dpy_, config, EGL_WIDTH, &width);
    eglGetConfigAttrib(dpy_, config, EGL_HEIGHT, &height);

    set_width(width);
    set_height(height);

    ctx_ = eglCreateContext(dpy_, config, NULL, NULL);
    surface_ = eglCreateWindowSurface(dpy_, config, 0, NULL);
    eglMakeCurrent(dpy_, surface_, surface_, ctx_);
    return true;
}

bool PSPWindow::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);

    set_has_context(true); //Mark that we have a valid GL context
    return true;
}

void PSPWindow::destroy_window() {
    eglTerminate(dpy_);
}

void PSPWindow::check_events() {

}

void PSPWindow::initialize_input_controller(InputState &controller) {

}

std::shared_ptr<SoundDriver> PSPWindow::create_sound_driver(const std::string& from_config) {
    _S_UNUSED(from_config);

    S_DEBUG("Null sound driver activated");
    return std::make_shared<NullSoundDriver>(this);
}

}
