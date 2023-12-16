#include <android/input.h>
#include <GLES/gl.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/window.h>
#include <android/native_window.h>
#include "android_window.h"

#include "../../sound/drivers/null_sound_driver.h"
#include "../../renderers/renderer_config.h"
#include "../../application.h"

/* Working with Android is a bit different to other platforms
 * this is the main entry point of the native activity
 * and from there we call the user-defined simulant_main()
 */


struct KeyboardEvent {
    int action;
    int key;
};

struct TouchEvent {
    int pointer_id;
    int action;
    float x;
    float y;
    float pressure;
};

struct AndroidEvent {
    int type;
    union {
        KeyboardEvent key;
        TouchEvent touch;
    };
};

static std::queue<int32_t> COMMANDS;
static std::queue<AndroidEvent> EVENTS;


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

    auto getX = [window, evt](int index) -> float {
        float x = AMotionEvent_getX(evt, index);

        if(window->width() <= 1) {
            x = 0.5f;
        } else {
            x /= float(window->width() - 1);
        }

        return x;
    };

    auto getY = [window, evt](int index) -> float {
        float y = AMotionEvent_getY(evt, index);

        if(window->height() <= 1) {
            y = 0.5f;
        } else {
            y /= float(window->height() - 1);
        }

        return y;
    };


    AndroidEvent out;
    out.type = AInputEvent_getType(evt);
    if(out.type == AINPUT_EVENT_TYPE_KEY) {
        out.key.action = AKeyEvent_getAction(evt);
        out.key.key = AKeyEvent_getKeyCode(evt);
        EVENTS.push(out);
    } else if(out.type == AINPUT_EVENT_TYPE_MOTION) {
        for(std::size_t i = 0; i < AMotionEvent_getPointerCount(evt); ++i) {
            out.touch.pointer_id = AMotionEvent_getPointerId(evt, i);
            out.touch.action = AMotionEvent_getAction(evt);
            out.touch.x = getX(i);
            out.touch.y = getY(i);
            out.touch.pressure = std::min(
                AMotionEvent_getPressure(evt, i), 1.0f
            );
            EVENTS.push(out);
        }
    }



    return 1;
}

AndroidWindow::AndroidWindow() {

}

void AndroidWindow::on_application_set(Application* app) {
    android_app* aapp = (android_app*) app->platform_state();
    aapp->userData = this;
    aapp->onAppCmd = android_handle_command;
    aapp->onInputEvent = android_handle_input;

    ANativeActivity_setWindowFlags(aapp->activity, AWINDOW_FLAG_FULLSCREEN, 0);

    /* Set the orientation. FIXME: Use the config! */
    JNIEnv* jni;
    aapp->activity->vm->AttachCurrentThread(&jni, NULL);
    auto clazz = jni->GetObjectClass(aapp->activity->clazz);
    auto methodID = jni->GetMethodID(clazz, "setRequestedOrientation", "(I)V");

    const int landscape = 0;
    // const int portrait = 1;
    jni->CallVoidMethod(aapp->activity->clazz, methodID, landscape);

    auto get_window = jni->GetMethodID(clazz, "getWindow", "()Landroid/view/Window;");
    auto window_class = jni->FindClass("android/view/Window");
    auto view_class = jni->FindClass("android/view/View");
    auto get_decor_view = jni->GetMethodID(window_class, "getDecorView", "()Landroid/view/View;");

    auto setSystemUiVisibility = jni->GetMethodID(view_class, "setSystemUiVisibility", "(I)V");

    auto window = jni->CallObjectMethod(aapp->activity->clazz, get_window);
    auto decor_view = jni->CallObjectMethod(window, get_decor_view);

    auto id_SYSTEM_UI_FLAG_LAYOUT_STABLE = jni->GetStaticFieldID(view_class, "SYSTEM_UI_FLAG_LAYOUT_STABLE", "I");
    auto id_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = jni->GetStaticFieldID(view_class, "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION", "I");
    auto id_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = jni->GetStaticFieldID(view_class, "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN", "I");
    auto id_SYSTEM_UI_FLAG_HIDE_NAVIGATION = jni->GetStaticFieldID(view_class, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I");
    auto id_SYSTEM_UI_FLAG_FULLSCREEN = jni->GetStaticFieldID(view_class, "SYSTEM_UI_FLAG_FULLSCREEN", "I");
    auto id_SYSTEM_UI_FLAG_IMMERSIVE_STICKY = jni->GetStaticFieldID(view_class, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I");

    const int flag_SYSTEM_UI_FLAG_LAYOUT_STABLE = jni->GetStaticIntField(view_class, id_SYSTEM_UI_FLAG_LAYOUT_STABLE);
    const int flag_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = jni->GetStaticIntField(view_class, id_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
    const int flag_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = jni->GetStaticIntField(view_class, id_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
    const int flag_SYSTEM_UI_FLAG_HIDE_NAVIGATION = jni->GetStaticIntField(view_class, id_SYSTEM_UI_FLAG_HIDE_NAVIGATION);
    const int flag_SYSTEM_UI_FLAG_FULLSCREEN = jni->GetStaticIntField(view_class, id_SYSTEM_UI_FLAG_FULLSCREEN);
    const int flag_SYSTEM_UI_FLAG_IMMERSIVE_STICKY = jni->GetStaticIntField(view_class, id_SYSTEM_UI_FLAG_IMMERSIVE_STICKY);

    const int flag =
        flag_SYSTEM_UI_FLAG_LAYOUT_STABLE |
        flag_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
        flag_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
        flag_SYSTEM_UI_FLAG_HIDE_NAVIGATION |
        flag_SYSTEM_UI_FLAG_FULLSCREEN |
        flag_SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

    jni->CallVoidMethod(decor_view, setSystemUiVisibility, flag);

    jni->DeleteLocalRef(window);
    jni->DeleteLocalRef(decor_view);
    aapp->activity->vm->DetachCurrentThread();
}

void AndroidWindow::swap_buffers() {
    if(dpy_ != EGL_NO_DISPLAY && has_focus()) {
        eglSwapBuffers(dpy_, surface_);
    }
}

void AndroidWindow::create_egl_surface(android_app* aapp) {
    EGLint width, height;

    surface_ = eglCreateWindowSurface(dpy_, config_, aapp->window, NULL);
    eglQuerySurface(dpy_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(dpy_, surface_, EGL_HEIGHT, &height);

    set_width(width);
    set_height(height);
}

bool AndroidWindow::_init_window() {
    android_app* aapp = (android_app*) application->platform_state();

    if(!aapp) {
        S_ERROR("No platform state?");
        return false;
    }

    while(!aapp->window) {
        thread::sleep(10);
        check_events();
        S_DEBUG("No native window yet...");
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
            config_ = cfg;
            break;
        }
    }

    if(!config_) {
        config_ = supported_configs[0];
    }

    if(!config_) {
        S_ERROR("Unable to find supported EGL config");
        return false;
    }

    EGLint format;

    eglGetConfigAttrib(dpy_, config_, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(aapp->window, 0, 0, format);

    create_egl_surface(aapp);

    const EGLint context_attribs [] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    ctx_ = eglCreateContext(dpy_, config_, NULL, context_attribs);
    if(eglMakeCurrent(dpy_, surface_, surface_, ctx_) == EGL_FALSE) {
        S_ERROR("Error making the context current");
        return false;
    }

    S_DEBUG("GL initialized, initializing the context");

    return true;
}

bool AndroidWindow::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);
    set_has_context(true);
    return true;
}

void AndroidWindow::destroy_window() {
    S_DEBUG("Destroying the EGL display");
    eglTerminate(dpy_);
    dpy_ = EGL_NO_DISPLAY;
}

static const KeyboardCode KEYCODE_MAPPING[256] = {
    KEYBOARD_CODE_NONE, // AKEYCODE_UNKNOWN
    KEYBOARD_CODE_LEFT,
    KEYBOARD_CODE_RIGHT,
    KEYBOARD_CODE_NONE, // HOME
    KEYBOARD_CODE_NONE, // BACK
    KEYBOARD_CODE_NONE, // CALL
    KEYBOARD_CODE_NONE, // END CALL
    KEYBOARD_CODE_0,
    KEYBOARD_CODE_1,
    KEYBOARD_CODE_2,
    KEYBOARD_CODE_3,
    KEYBOARD_CODE_4,
    KEYBOARD_CODE_5,
    KEYBOARD_CODE_6,
    KEYBOARD_CODE_7,
    KEYBOARD_CODE_8,
    KEYBOARD_CODE_9,
    KEYBOARD_CODE_KP_MULTIPLY,  // STAR
    KEYBOARD_CODE_KP_HASH, // POUND
    KEYBOARD_CODE_UP,
    KEYBOARD_CODE_DOWN,
    KEYBOARD_CODE_LEFT,
    KEYBOARD_CODE_RIGHT,
    KEYBOARD_CODE_NONE, // DPAD CENTER
    KEYBOARD_CODE_VOLUMEUP, // VOL UP
    KEYBOARD_CODE_VOLUMEDOWN, // VOL DOWN
    KEYBOARD_CODE_POWER,
    KEYBOARD_CODE_NONE, // CAMERA
    KEYBOARD_CODE_NONE, // CLEAR
    KEYBOARD_CODE_A,
    KEYBOARD_CODE_B,
    KEYBOARD_CODE_C,
    KEYBOARD_CODE_D,
    KEYBOARD_CODE_E,
    KEYBOARD_CODE_F,
    KEYBOARD_CODE_G,
    KEYBOARD_CODE_H,
    KEYBOARD_CODE_I,
    KEYBOARD_CODE_J,
    KEYBOARD_CODE_K,
    KEYBOARD_CODE_L,
    KEYBOARD_CODE_M,
    KEYBOARD_CODE_N,
    KEYBOARD_CODE_O,
    KEYBOARD_CODE_P,
    KEYBOARD_CODE_Q,
    KEYBOARD_CODE_R,
    KEYBOARD_CODE_S,
    KEYBOARD_CODE_T,
    KEYBOARD_CODE_U,
    KEYBOARD_CODE_V,
    KEYBOARD_CODE_W,
    KEYBOARD_CODE_X,
    KEYBOARD_CODE_Y,
    KEYBOARD_CODE_Z,
    KEYBOARD_CODE_COMMA,
    KEYBOARD_CODE_PERIOD,
    KEYBOARD_CODE_LALT,
    KEYBOARD_CODE_RALT,
    KEYBOARD_CODE_LSHIFT,
    KEYBOARD_CODE_RSHIFT,
    KEYBOARD_CODE_TAB,
    KEYBOARD_CODE_SPACE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE, // EXPLORER
    KEYBOARD_CODE_NONE, // ENVELOPE
    KEYBOARD_CODE_RETURN,
    KEYBOARD_CODE_DELETE,
    KEYBOARD_CODE_GRAVE,
    KEYBOARD_CODE_MINUS,
    KEYBOARD_CODE_EQUALS,
    KEYBOARD_CODE_LEFTBRACKET,
    KEYBOARD_CODE_RIGHTBRACKET,
    KEYBOARD_CODE_BACKSLASH,
    KEYBOARD_CODE_SEMICOLON,
    KEYBOARD_CODE_APOSTROPHE,
    KEYBOARD_CODE_SLASH,
    KEYBOARD_CODE_NONE,  // AT
    KEYBOARD_CODE_NONE,  // NUM
    KEYBOARD_CODE_NONE, // HEADSETHOOK
    KEYBOARD_CODE_NONE, // FOCUS
    KEYBOARD_CODE_KP_PLUS,
    KEYBOARD_CODE_NONE, // MENU
    KEYBOARD_CODE_NONE, // NOTIFICATION
    KEYBOARD_CODE_NONE, // SEARCH
    KEYBOARD_CODE_AUDIOPLAY,
    KEYBOARD_CODE_STOP,
    KEYBOARD_CODE_AUDIONEXT,
    KEYBOARD_CODE_AUDIOPREV,
    KEYBOARD_CODE_NONE,  // REWIND
    KEYBOARD_CODE_NONE, // FAST FORWARD
    KEYBOARD_CODE_AUDIOMUTE,
    KEYBOARD_CODE_PAGEUP,
    KEYBOARD_CODE_PAGEDOWN,
    KEYBOARD_CODE_NONE, // PICT SYMBOLS
    KEYBOARD_CODE_NONE, // SWITCH CHARSET
    KEYBOARD_CODE_KP_A, //
    KEYBOARD_CODE_KP_B,
    KEYBOARD_CODE_KP_C,
    KEYBOARD_CODE_KP_D, // X
    KEYBOARD_CODE_KP_E, // Y
    KEYBOARD_CODE_KP_F, // Z
    KEYBOARD_CODE_NONE, // L1
    KEYBOARD_CODE_NONE, // R1
    KEYBOARD_CODE_NONE, // L2
    KEYBOARD_CODE_NONE, // R2
    KEYBOARD_CODE_NONE, // THUMBL
    KEYBOARD_CODE_NONE, // THUMBR
    KEYBOARD_CODE_NONE, // START
    KEYBOARD_CODE_NONE, // SELECT
    KEYBOARD_CODE_NONE, // MODE
    KEYBOARD_CODE_ESCAPE,
    KEYBOARD_CODE_NONE, // FWD DELETE
    KEYBOARD_CODE_LCTRL,
    KEYBOARD_CODE_RCTRL,
    KEYBOARD_CODE_CAPSLOCK,
    KEYBOARD_CODE_SCROLLLOCK,
    KEYBOARD_CODE_LGUI,
    KEYBOARD_CODE_RGUI,
    KEYBOARD_CODE_NONE, // FUNCTION
    KEYBOARD_CODE_SYSREQ,
    KEYBOARD_CODE_NONE,  // BREAK
    KEYBOARD_CODE_HOME,
    KEYBOARD_CODE_END,
    KEYBOARD_CODE_INSERT,
    KEYBOARD_CODE_NONE, // FORWARD
    KEYBOARD_CODE_NONE, // PLAY
    KEYBOARD_CODE_NONE, // PAUSE
    KEYBOARD_CODE_NONE, // CLOSE
    KEYBOARD_CODE_NONE, // EJECT
    KEYBOARD_CODE_NONE, // RECORD
    KEYBOARD_CODE_F1,
    KEYBOARD_CODE_F2,
    KEYBOARD_CODE_F3,
    KEYBOARD_CODE_F4,
    KEYBOARD_CODE_F5,
    KEYBOARD_CODE_F6,
    KEYBOARD_CODE_F7,
    KEYBOARD_CODE_F8,
    KEYBOARD_CODE_F9,
    KEYBOARD_CODE_F10,
    KEYBOARD_CODE_F11,
    KEYBOARD_CODE_F12,
    KEYBOARD_CODE_NONE, // NUMLOCK
    KEYBOARD_CODE_KP_0,
    KEYBOARD_CODE_KP_1,
    KEYBOARD_CODE_KP_2,
    KEYBOARD_CODE_KP_3,
    KEYBOARD_CODE_KP_4,
    KEYBOARD_CODE_KP_5,
    KEYBOARD_CODE_KP_6,
    KEYBOARD_CODE_KP_7,
    KEYBOARD_CODE_KP_8,
    KEYBOARD_CODE_KP_9,
    KEYBOARD_CODE_KP_DIVIDE,
    KEYBOARD_CODE_KP_MULTIPLY,
    KEYBOARD_CODE_KP_MINUS,
    KEYBOARD_CODE_KP_PLUS,
    KEYBOARD_CODE_KP_PERIOD,
    KEYBOARD_CODE_KP_COMMA,
    KEYBOARD_CODE_KP_ENTER,
    KEYBOARD_CODE_KP_EQUALS,
    KEYBOARD_CODE_KP_LEFTPAREN,
    KEYBOARD_CODE_KP_RIGHTPAREN,
    KEYBOARD_CODE_MUTE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
    KEYBOARD_CODE_NONE,
};

static struct {
    float x;
    float y;
    float pressure;
} FINGER_STATES[5] = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
};

const static int32_t TOUCH_MOUSE_ID = 0;

void AndroidWindow::check_events() {
    android_app* aapp = (android_app*) get_app()->platform_state();

    struct android_poll_source* source;
    int events = 0;
    ALooper_pollAll(0, nullptr, &events, (void**) &source);

    if (source != nullptr) {
        source->process(aapp, source);
    }

    if(aapp->destroyRequested) {
        S_DEBUG("Destroy request received, shutting down");
        get_app()->stop_running();
        return;
    }

    while(!COMMANDS.empty()) {
        int32_t cmd = COMMANDS.front();
        COMMANDS.pop();

        switch(cmd) {
            case APP_CMD_SAVE_STATE:
                S_INFO("CMD: Save state");
                break;

            case APP_CMD_INIT_WINDOW:
                S_INFO("CMD: Init window");
                if(dpy_ != EGL_NO_DISPLAY && surface_ == EGL_NO_SURFACE && ctx_ != EGL_NO_CONTEXT) {
                    // Rebind the context to the display/surface
                    create_egl_surface(aapp);
                    eglMakeCurrent(dpy_, surface_, surface_, ctx_);
                }

                break;
            case APP_CMD_TERM_WINDOW:
                S_INFO("CMD: Term window");

                if(dpy_ != EGL_NO_DISPLAY && surface_ != EGL_NO_SURFACE && ctx_ != EGL_NO_CONTEXT) {
                    eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    eglDestroySurface(dpy_, surface_);
                    surface_ = EGL_NO_SURFACE;
                }

                break;
            case APP_CMD_DESTROY:
                set_has_context(false);
                destroy_window();
                break;
            case APP_CMD_GAINED_FOCUS:
                S_INFO("CMD: Focus gained");
               // {
               //     //See Window::context_lock_ for details
               //     thread::Lock<thread::Mutex> context_lock(this->context_lock());
               //     set_has_context(true);
               // }
                //FIXME: Reload textures and shaders
                set_has_focus(true);
                break;
            case APP_CMD_LOST_FOCUS:
                S_INFO("CMD: Focus lost");
                set_has_focus(false);
               // {
               //     //See Window::context_lock_ for details
               //     thread::Lock<thread::Mutex> context_lock(this->context_lock());
               //     set_has_context(false);
               // }
                break;
            default:
                break;
        }
    }

    while(!EVENTS.empty()) {
        AndroidEvent evt = EVENTS.front();
        EVENTS.pop();

        switch(evt.type) {
            case AINPUT_EVENT_TYPE_KEY: {
                auto key = KEYCODE_MAPPING[evt.key.key];
                if(evt.key.action == AKEY_EVENT_ACTION_UP) {
                    input_state->_handle_key_up(0, key);
                    on_key_up(key, ModifierKeyState());
                } else if(evt.key.action == AKEY_EVENT_ACTION_DOWN) {
                    input_state->_handle_key_down(0, key);
                    on_key_down(key, ModifierKeyState());
                } else {}
            } break;
            case AINPUT_EVENT_TYPE_MOTION: {
                auto pointer = evt.touch.pointer_id;
                if(pointer < 5) {
                    float x = evt.touch.x;
                    float y = evt.touch.y;

                    if(evt.touch.action == AMOTION_EVENT_ACTION_DOWN) {
                        S_INFO("DOWN: {0} ({1}, {2})", pointer, x, y);
                        on_finger_down(
                            pointer,
                            x,
                            1.0f - y,
                            evt.touch.pressure
                        );

                        on_mouse_down(
                            TOUCH_MOUSE_ID,
                            pointer,
                            x * float(width()),
                            (1.0f - y) * float(height()),
                            true
                        );

                        FINGER_STATES[pointer].x = x;
                        FINGER_STATES[pointer].y = 1.0f - y;
                        FINGER_STATES[pointer].pressure = evt.touch.pressure;
                    } else if(evt.touch.action == AMOTION_EVENT_ACTION_UP) {
                        S_INFO("UP: {0} ({1}, {2})", pointer, x, y);
                        on_finger_up(pointer, x, 1.0f - y);

                        on_mouse_up(
                            TOUCH_MOUSE_ID,
                            pointer,
                            x * float(width()),
                            (1.0f - y) * float(height()),
                            true
                        );

                        FINGER_STATES[pointer].x = x;
                        FINGER_STATES[pointer].y = 1.0f - y;
                        FINGER_STATES[pointer].pressure = 0.0f;
                    } else {
                        S_INFO("MOVE: {0}", pointer);
                        float dx = x - FINGER_STATES[pointer].x;
                        float dy = y - FINGER_STATES[pointer].y;

                        on_finger_motion(pointer, x, 1.0f - y, dx, dy);
                        on_mouse_move(
                            TOUCH_MOUSE_ID,
                            x * float(width()),
                            (1.0f - y) * float(height()),
                            true
                        );

                        FINGER_STATES[pointer].x = x;
                        FINGER_STATES[pointer].y = 1.0f - y;
                        FINGER_STATES[pointer].pressure = evt.touch.pressure;
                    }
                }
            }

            default:
                S_INFO("Event type: {0}", evt.type);
            break;
        }
    }
}

void AndroidWindow::initialize_input_controller(InputState &controller) {
    KeyboardDeviceInfo keyboard;
    keyboard.id = 0;
    keyboard.type = KEYBOARD_TYPE_SOFTWARE;
    controller._update_keyboard_devices({keyboard});
}

std::shared_ptr<SoundDriver> AndroidWindow::create_sound_driver(const std::string& from_config) {
    _S_UNUSED(from_config);

    S_DEBUG("Null sound driver activated");
    return std::make_shared<NullSoundDriver>(this);
}

}
