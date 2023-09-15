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

static android_app* ANDROID_APP = nullptr;
static std::queue<int32_t> COMMANDS;
static std::queue<AndroidInputEvent> EVENTS;

int __attribute__((weak)) simulant_main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);
    return 0;
}

/* Stores the android_app globally and then calls the
 * user-defined main() entrypoint */
void android_main(struct android_app* state) {
    ANDROID_APP = state;
    simulant_main(0, NULL);
}

namespace smlt {

static const EGLint attrib_list [] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 0,
    EGL_DEPTH_SIZE, 0,
    EGL_NONE
};

static void android_handle_command(struct android_app* app, int32_t cmd) {
    AndroidWindow* window = (AndroidWindow*) app->userData;
    assert(window);

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
    ANDROID_APP->userData = this;
    ANDROID_APP->onAppCmd = android_handle_command;
    ANDROID_APP->onInputEvent = android_handle_input;
}

void AndroidWindow::swap_buffers() {
    eglSwapBuffers(dpy_, surface_);
}

bool AndroidWindow::_init_window() {
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

bool AndroidWindow::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);

    set_has_context(true); //Mark that we have a valid GL context
    return true;
}

void AndroidWindow::destroy_window() {
    eglTerminate(dpy_);
}

void AndroidWindow::check_events() {
    if(ANDROID_APP->destroyRequested) {
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
                break;
            case APP_CMD_TERM_WINDOW:
                // The window is being hidden or closed, clean it up.
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
