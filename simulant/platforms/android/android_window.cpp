#include <android/input.h>
#include <GLES/gl.h>
#include <android_native_app_glue.h>

#include "android_window.h"

#include "../../sound/drivers/null_sound_driver.h"
#include "../../renderers/renderer_config.h"
#include "../../application.h"

/* Working with Android is a bit different to other platforms
 * this is the main entry point of the native activity
 * and from there we call the user-defined simulant_main()
 */

struct AndroidInputEvent {
    int32_t type;
    int32_t source;
};

static std::queue<int32_t> COMMANDS;
static std::queue<AndroidInputEvent> EVENTS;

namespace smlt {

static const EGLint attrib_list [] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_NONE
};

static void android_handle_command(struct android_app* app, int32_t cmd) {
    AndroidWindow* window = (AndroidWindow*) app->userData;
    assert(window);

    S_INFO("Received command: {0}", cmd);
    COMMANDS.push(cmd);
}

static int android_handle_input(struct android_app* app, AInputEvent* evt) {
    AndroidWindow* window = (AndroidWindow*) app->userData;
    assert(window);

    AndroidInputEvent aevt;
    aevt.type = AInputEvent_getType(evt);
    aevt.source = AInputEvent_getSource(evt);

    EVENTS.push(aevt);
    return 1;
}

AndroidWindow::AndroidWindow() {

}

void AndroidWindow::on_application_set(Application* app) {
    android_app* aapp = (android_app*) app->platform_state();
    aapp->userData = this;
    aapp->onAppCmd = android_handle_command;
    aapp->onInputEvent = android_handle_input;
}

void AndroidWindow::swap_buffers() {
    if(dpy_ != EGL_NO_DISPLAY) {
        eglSwapBuffers(dpy_, surface_);
    }
}

bool AndroidWindow::_init_window() {
    android_app* aapp = (android_app*) app->platform_state();

    if(!aapp) {
        S_ERROR("No platform state?");
        return false;
    }

    if(!aapp->window) {
        S_DEBUG("No native window yet, so nothing to do!");
        return true;
    }

    if(dpy_ != EGL_NO_DISPLAY) {
        S_DEBUG("Not initializing window, already initialized");
        return false;
    }

    dpy_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(dpy_ == EGL_NO_DISPLAY) {
        S_ERROR("Couldn't get egl display");
        return false;
    }

    if(eglInitialize(dpy_, NULL, NULL) == EGL_FALSE) {
        S_ERROR("Couldn't initialize EGL");
        return false;
    }

    EGLConfig config = nullptr;
    EGLint num_configs;

    eglChooseConfig(dpy_, attrib_list, nullptr, 0, &num_configs);
    if(!num_configs) {
        return false;
    }

    std::vector<EGLConfig> supported_configs(num_configs);
    eglChooseConfig(
        dpy_, attrib_list, &supported_configs[0], num_configs,
        &num_configs
    );

    EGLint max_depth = 0;
    for(auto& cfg: supported_configs) {
        EGLint r, g, b, a, d;
        eglGetConfigAttrib(dpy_, cfg, EGL_RED_SIZE, &r);
        eglGetConfigAttrib(dpy_, cfg, EGL_GREEN_SIZE, &g);
        eglGetConfigAttrib(dpy_, cfg, EGL_BLUE_SIZE, &b);
        eglGetConfigAttrib(dpy_, cfg, EGL_ALPHA_SIZE, &a);
        eglGetConfigAttrib(dpy_, cfg, EGL_DEPTH_SIZE, &d);

        if(r == 8 && g == 8 && b == 8 && d >= max_depth) {
            S_DEBUG("Found config: {0}:{1}:{2}:{3} {4}", r, g, b, a, d);
            max_depth = d;
            config = cfg;
            break;
        }
    }

    if(!config) {
        config = supported_configs[0];
    }

    if(!config) {
        S_ERROR("Unable to find supported EGL config");
        return false;
    }

    EGLint width, height, format;

    eglGetConfigAttrib(dpy_, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(aapp->window, 0, 0, format);

    surface_ = eglCreateWindowSurface(dpy_, config, aapp->window, NULL);
    eglQuerySurface(dpy_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(dpy_, surface_, EGL_HEIGHT, &height);

    set_width(width);
    set_height(height);

    ctx_ = eglCreateContext(dpy_, config, NULL, NULL);
    if(eglMakeCurrent(dpy_, surface_, surface_, ctx_) == EGL_FALSE) {
        S_ERROR("Error making the context current");
        return false;
    }

    /* We manually init the context here, rather than letting the Window
     * do it as with other platforms */
    if(renderer_) {
        renderer_->init_context();
    }

    return true;
}

bool AndroidWindow::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);
    return true;
}

void AndroidWindow::destroy_window() {
    eglTerminate(dpy_);
    dpy_ = EGL_NO_DISPLAY;
}

void AndroidWindow::check_events() {
    android_app* aapp = (android_app*) get_app()->platform_state();
    if(aapp->destroyRequested) {
        get_app()->stop_running();
        return;
    }

    while(!COMMANDS.empty()) {
        int32_t cmd = COMMANDS.front();
        COMMANDS.pop();

        switch(cmd) {
            case APP_CMD_SAVE_STATE:
                break;
            case APP_CMD_INIT_WINDOW:
                // The window is being shown, get it ready.
                _init_window();
                set_has_context(true);
                break;
            case APP_CMD_TERM_WINDOW:
                // The window is being hidden or closed, clean it up.
                set_has_context(false);
                destroy_window();
                break;
            case APP_CMD_GAINED_FOCUS:
                {
                    //See Window::context_lock_ for details
                    thread::Lock<thread::Mutex> context_lock(this->context_lock());
                    set_has_context(true);
                }
                //FIXME: Reload textures and shaders
                set_has_focus(true);
                break;
            case APP_CMD_LOST_FOCUS:
                set_has_focus(false);
                {
                    //See Window::context_lock_ for details
                    thread::Lock<thread::Mutex> context_lock(this->context_lock());
                    set_has_context(false);
                }
                break;
            default:
                break;
        }
    }

    while(!EVENTS.empty()) {
        AndroidInputEvent evt = EVENTS.front();
        EVENTS.pop();

        switch(evt.type) {
            default:
            break;
        }
    }
}

void AndroidWindow::initialize_input_controller(InputState &controller) {

}

std::shared_ptr<SoundDriver> AndroidWindow::create_sound_driver(const std::string& from_config) {
    _S_UNUSED(from_config);

    S_DEBUG("Null sound driver activated");
    return std::make_shared<NullSoundDriver>(this);
}

}
