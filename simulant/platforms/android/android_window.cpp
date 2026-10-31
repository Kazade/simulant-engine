#include <algorithm>
#include <cmath>
#include <cstring>
#include <android/input.h>
#include <GLES/gl.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/window.h>
#include <android/native_window.h>
#include "android_window.h"

#include "../../sound/drivers/openal_sound_driver.h"
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

const static MouseID TOUCH_MOUSE_ID(0);

static const EGLint attrib_list [] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_NONE
};

static void android_handle_command(struct android_app* app, int32_t cmd) {
    S_INFO("Received command: {0}", cmd);

    /* The window has to be dealt with before this returns, rather than
     * queued with everything else: once it has, native_app_glue clears
     * app->window and lets Android destroy the old one, so a surface still
     * using it would be drawn to after it's gone */
    AndroidWindow* window = (AndroidWindow*) app->userData;
    if(window && cmd == APP_CMD_INIT_WINDOW) {
        window->on_native_window_created(app);
    } else if(window && cmd == APP_CMD_TERM_WINDOW) {
        window->on_native_window_destroyed();
    } else {
        COMMANDS.push(cmd);
    }
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

        /* Ignore certain key events */
        switch(out.key.key) {
            case AKEYCODE_VOLUME_UP:
            case AKEYCODE_VOLUME_DOWN:
            case AKEYCODE_CAMERA:
            case AKEYCODE_ZOOM_IN:
            case AKEYCODE_ZOOM_OUT:
                return 0;
            default:
                break;
        }

        EVENTS.push(out);
    } else if(out.type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(evt);
        std::size_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        out.touch.action = action & AMOTION_EVENT_ACTION_MASK;

        auto push = [&](std::size_t i) {
            out.touch.pointer_id = AMotionEvent_getPointerId(evt, i);
            out.touch.x = getX(i);
            out.touch.y = getY(i);
            out.touch.pressure = std::min(
                AMotionEvent_getPressure(evt, i), 1.0f
            );
            EVENTS.push(out);
        };

        /* A move is for every finger that's down, and a cancel (e.g. when the
         * system takes over a gesture) lifts all of them. The rest are for
         * the one finger the action's index says */
        if(out.touch.action == AMOTION_EVENT_ACTION_MOVE ||
           out.touch.action == AMOTION_EVENT_ACTION_CANCEL) {
            std::size_t count = AMotionEvent_getPointerCount(evt);
            for(std::size_t i = 0; i < count; ++i) {
                push(i);
            }
        } else {
            push(index);
        }
    }

    return 1;
}

AndroidWindow::AndroidWindow() {

}

void AndroidWindow::on_application_set(Application* app) {
    android_app* aapp = (android_app*) app->platform_state();
    aapp->userData = this;

    /* The process can outlive the activity, and run the game again for a new
     * one. Anything left over from the last run (e.g. its APP_CMD_DESTROY,
     * which is never processed as it stops on destroyRequested) mustn't be
     * acted on now, or the new activity would be finished straight away */
    COMMANDS = std::queue<int32_t>();
    EVENTS = std::queue<AndroidEvent>();
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

    /* Set the volume control to STREAM_MUSIC */
    auto setVolumeControlStream = jni->GetMethodID(clazz, "setVolumeControlStream", "(I)V");
    auto STREAM_MUSIC = 0x00000003; // Let's hope this doesn't change
    jni->CallVoidMethod(aapp->activity->clazz, setVolumeControlStream, STREAM_MUSIC);

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

void AndroidWindow::do_swap_buffers() {
    /* Whether there's a surface is what matters, not focus: the window can
     * be on screen without it, e.g. under something translucent (like
     * Google Play's purchase sheet) or in split screen, and focus doesn't
     * always come back cleanly afterwards, which would leave the game
     * running behind a frozen picture */
    if(dpy_ != EGL_NO_DISPLAY && surface_ != EGL_NO_SURFACE) {
        static bool warned = false;
        if(eglSwapBuffers(dpy_, surface_) == EGL_FALSE && !warned) {
            S_WARN("eglSwapBuffers failed: {0}", eglGetError());
            warned = true;
        }
    }
}

void AndroidWindow::create_egl_surface(android_app* aapp) {
    EGLint width, height;

    surface_ = eglCreateWindowSurface(dpy_, config_, aapp->window, NULL);
    if(surface_ == EGL_NO_SURFACE) {
        S_ERROR("Couldn't create a window surface: {0}", eglGetError());
        return;
    }

    eglQuerySurface(dpy_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(dpy_, surface_, EGL_HEIGHT, &height);

    set_width(width);
    set_height(height);
}

void AndroidWindow::on_native_window_created(android_app* aapp) {
    S_INFO("Native window created");

    /* At startup, _init_window sets everything up once there's a window */
    if(dpy_ == EGL_NO_DISPLAY || ctx_ == EGL_NO_CONTEXT || !aapp->window) {
        return;
    }

    if(surface_ != EGL_NO_SURFACE) {
        on_native_window_destroyed();
    }

    EGLint format;
    eglGetConfigAttrib(dpy_, config_, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(aapp->window, 0, 0, format);

    create_egl_surface(aapp);
    if(surface_ != EGL_NO_SURFACE &&
       eglMakeCurrent(dpy_, surface_, surface_, ctx_) == EGL_FALSE) {
        S_ERROR("Couldn't make the context current: {0}", eglGetError());
    }

    update_safe_area();
}

void AndroidWindow::on_native_window_destroyed() {
    S_INFO("Native window destroyed");

    if(dpy_ == EGL_NO_DISPLAY || surface_ == EGL_NO_SURFACE) {
        return;
    }

    /* Keep the context current if it can be without a surface, otherwise
     * nothing can be */
    if(!surfaceless_ ||
       eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx_) == EGL_FALSE) {
        eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    eglDestroySurface(dpy_, surface_);
    surface_ = EGL_NO_SURFACE;
}

void AndroidWindow::update_safe_area() {
    android_app* aapp = (android_app*) smlt::get_app()->platform_state();
    if(!aapp) {
        return;
    }

    JavaVM* vm = aapp->activity->vm;
    JNIEnv* jni = nullptr;

    /* Only detach afterwards if we're the ones that attached */
    bool attached = false;
    if(vm->GetEnv((void**) &jni, JNI_VERSION_1_6) != JNI_OK) {
        if(vm->AttachCurrentThread(&jni, nullptr) != JNI_OK) {
            return;
        }
        attached = true;
    }

    SafeArea insets;

    auto activity_class = jni->GetObjectClass(aapp->activity->clazz);
    auto get_window = jni->GetMethodID(activity_class, "getWindow", "()Landroid/view/Window;");
    auto window_class = jni->FindClass("android/view/Window");
    auto get_decor_view = jni->GetMethodID(window_class, "getDecorView", "()Landroid/view/View;");
    auto view_class = jni->FindClass("android/view/View");
    auto get_root_window_insets = jni->GetMethodID(view_class, "getRootWindowInsets", "()Landroid/view/WindowInsets;");

    auto window = jni->CallObjectMethod(aapp->activity->clazz, get_window);
    auto decor_view = (window) ? jni->CallObjectMethod(window, get_decor_view) : nullptr;

    /* Null until the view's been laid out */
    auto window_insets = (decor_view) ? jni->CallObjectMethod(decor_view, get_root_window_insets) : nullptr;

    if(window_insets) {
        auto insets_class = jni->FindClass("android/view/WindowInsets");

        auto get_cutout = jni->GetMethodID(insets_class, "getDisplayCutout", "()Landroid/view/DisplayCutout;");
        auto cutout = jni->CallObjectMethod(window_insets, get_cutout);
        if(cutout) {
            auto cutout_class = jni->FindClass("android/view/DisplayCutout");
            auto inset = [&](const char* name) -> uint16_t {
                auto method = jni->GetMethodID(cutout_class, name, "()I");
                return (uint16_t) std::max(0, (int) jni->CallIntMethod(cutout, method));
            };

            insets.left = inset("getSafeInsetLeft");
            insets.top = inset("getSafeInsetTop");
            insets.right = inset("getSafeInsetRight");
            insets.bottom = inset("getSafeInsetBottom");

            jni->DeleteLocalRef(cutout);
            jni->DeleteLocalRef(cutout_class);
        }

        /* Rounded corners (Android 12 onwards) cut into the edges near them.
         * Something square in a corner is clear of the curve once it's in
         * from both edges by r * (1 - 1/sqrt(2)) */
        if(AConfiguration_getSdkVersion(aapp->config) >= 31) {
            auto get_corner = jni->GetMethodID(insets_class, "getRoundedCorner", "(I)Landroid/view/RoundedCorner;");
            auto corner_class = jni->FindClass("android/view/RoundedCorner");
            auto get_radius = jni->GetMethodID(corner_class, "getRadius", "()I");

            auto corner_inset = [&](int position) -> uint16_t {
                auto corner = jni->CallObjectMethod(window_insets, get_corner, position);
                if(!corner) {
                    return 0;
                }

                int radius = jni->CallIntMethod(corner, get_radius);
                jni->DeleteLocalRef(corner);
                return (uint16_t) std::ceil(std::max(0, radius) * (1.0f - 0.70710678f));
            };

            // RoundedCorner.POSITION_TOP_LEFT, _TOP_RIGHT, _BOTTOM_RIGHT, _BOTTOM_LEFT
            uint16_t top_left = corner_inset(0);
            uint16_t top_right = corner_inset(1);
            uint16_t bottom_right = corner_inset(2);
            uint16_t bottom_left = corner_inset(3);

            insets.left = std::max({insets.left, top_left, bottom_left});
            insets.right = std::max({insets.right, top_right, bottom_right});
            insets.top = std::max({insets.top, top_left, top_right});
            insets.bottom = std::max({insets.bottom, bottom_left, bottom_right});

            jni->DeleteLocalRef(corner_class);
        }

        jni->DeleteLocalRef(insets_class);
        jni->DeleteLocalRef(window_insets);
    }

    /* e.g. a method that's missing on an older version of Android */
    if(jni->ExceptionCheck()) {
        jni->ExceptionClear();
    }

    if(decor_view) jni->DeleteLocalRef(decor_view);
    if(window) jni->DeleteLocalRef(window);
    jni->DeleteLocalRef(view_class);
    jni->DeleteLocalRef(window_class);
    jni->DeleteLocalRef(activity_class);

    if(attached) {
        vm->DetachCurrentThread();
    }

    if(insets != safe_area()) {
        S_INFO(
            "Safe area insets: left {0}, top {1}, right {2}, bottom {3}",
            insets.left, insets.top, insets.right, insets.bottom
        );
        set_safe_area(insets);
    }
}

namespace {

/* The JNI environment for the current thread, attaching it to Java for as
 * long as this is around if it isn't already */
class JavaThread {
public:
    JavaThread() {
        android_app* aapp = (android_app*) smlt::get_app()->platform_state();
        if(!aapp) {
            return;
        }

        vm_ = aapp->activity->vm;
        activity_ = aapp->activity->clazz;
        if(vm_->GetEnv((void**) &env_, JNI_VERSION_1_6) != JNI_OK) {
            if(vm_->AttachCurrentThread(&env_, nullptr) != JNI_OK) {
                env_ = nullptr;
                return;
            }
            attached_ = true;
        }
    }

    ~JavaThread() {
        if(env_ && env_->ExceptionCheck()) {
            env_->ExceptionClear();
        }

        if(attached_) {
            vm_->DetachCurrentThread();
        }
    }

    JNIEnv* env() const { return env_; }
    jobject activity() const { return activity_; }

    /* Clears any exception, returning true if there was one */
    bool failed() const {
        if(env_->ExceptionCheck()) {
            env_->ExceptionClear();
            return true;
        }
        return false;
    }

private:
    JavaVM* vm_ = nullptr;
    JNIEnv* env_ = nullptr;
    jobject activity_ = nullptr;
    bool attached_ = false;
};

}

void AndroidWindow::find_vibrator() const {
    if(vibrator_checked_) {
        return;
    }
    vibrator_checked_ = true;

    JavaThread java;
    JNIEnv* env = java.env();
    if(!env) {
        return;
    }

    /* context.getSystemService(Context.VIBRATOR_SERVICE). From Android 12
     * the vibrator's meant to come from the VibratorManager service, but
     * this still works and goes back further. */
    jclass activity_class = env->GetObjectClass(java.activity());
    jmethodID get_system_service = env->GetMethodID(
        activity_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring name = env->NewStringUTF("vibrator");
    jobject vibrator = env->CallObjectMethod(java.activity(), get_system_service, name);

    if(vibrator && !java.failed()) {
        jclass vibrator_class = env->FindClass("android/os/Vibrator");
        jmethodID has_vibrator = env->GetMethodID(vibrator_class, "hasVibrator", "()Z");
        jmethodID has_amplitude_control = env->GetMethodID(vibrator_class, "hasAmplitudeControl", "()Z");

        if(env->CallBooleanMethod(vibrator, has_vibrator) && !java.failed()) {
            vibrator_ = env->NewGlobalRef(vibrator);
            has_amplitude_control_ = env->CallBooleanMethod(vibrator, has_amplitude_control) && !java.failed();
        }

        env->DeleteLocalRef(vibrator_class);
    }

    S_INFO("Haptics: {0}{1}", vibrator_ ? "yes" : "no",
           has_amplitude_control_ ? " (with amplitude control)" : "");

    if(vibrator) env->DeleteLocalRef(vibrator);
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(activity_class);
}

bool AndroidWindow::has_haptics() const {
    find_vibrator();
    return vibrator_ != nullptr;
}

void AndroidWindow::start_haptics(NormalizedFloat strength, const Seconds& duration) {
    if(!has_haptics() || strength <= 0.0f) {
        return;
    }

    JavaThread java;
    JNIEnv* env = java.env();
    if(!env) {
        return;
    }

    jlong ms = std::max(jlong(1), jlong(std::round(duration.to_float() * 1000.0f)));

    /* Without amplitude control, VibrationEffect.DEFAULT_AMPLITUDE (-1) */
    jint amplitude = has_amplitude_control_ ?
        jint(std::max(1.0f, std::min(255.0f, std::round(strength * 255.0f)))) : -1;

    jclass effect_class = env->FindClass("android/os/VibrationEffect");
    jmethodID create_one_shot = env->GetStaticMethodID(
        effect_class, "createOneShot", "(JI)Landroid/os/VibrationEffect;");
    jobject effect = env->CallStaticObjectMethod(effect_class, create_one_shot, ms, amplitude);

    /* Tagged as a game's, so it follows the player's settings for those */
    const jint USAGE_GAME = 14;
    jclass builder_class = env->FindClass("android/media/AudioAttributes$Builder");
    jmethodID builder_init = env->GetMethodID(builder_class, "<init>", "()V");
    jmethodID set_usage = env->GetMethodID(
        builder_class, "setUsage", "(I)Landroid/media/AudioAttributes$Builder;");
    jmethodID build = env->GetMethodID(builder_class, "build", "()Landroid/media/AudioAttributes;");

    jobject builder = env->NewObject(builder_class, builder_init);
    jobject builder_again = env->CallObjectMethod(builder, set_usage, USAGE_GAME);
    jobject attributes = env->CallObjectMethod(builder, build);

    if(effect && attributes && !java.failed()) {
        jclass vibrator_class = env->FindClass("android/os/Vibrator");
        jmethodID vibrate = env->GetMethodID(
            vibrator_class, "vibrate",
            "(Landroid/os/VibrationEffect;Landroid/media/AudioAttributes;)V");
        env->CallVoidMethod(vibrator_, vibrate, effect, attributes);

        /* e.g. without the VIBRATE permission */
        if(java.failed()) {
            S_WARN("Couldn't vibrate the device");
        }

        env->DeleteLocalRef(vibrator_class);
    }

    if(attributes) env->DeleteLocalRef(attributes);
    if(builder_again) env->DeleteLocalRef(builder_again);
    if(builder) env->DeleteLocalRef(builder);
    env->DeleteLocalRef(builder_class);
    if(effect) env->DeleteLocalRef(effect);
    env->DeleteLocalRef(effect_class);
}

void AndroidWindow::stop_haptics() {
    if(!has_haptics()) {
        return;
    }

    JavaThread java;
    JNIEnv* env = java.env();
    if(!env) {
        return;
    }

    jclass vibrator_class = env->FindClass("android/os/Vibrator");
    jmethodID cancel = env->GetMethodID(vibrator_class, "cancel", "()V");
    env->CallVoidMethod(vibrator_, cancel);
    java.failed();
    env->DeleteLocalRef(vibrator_class);
}

void AndroidWindow::release_all_fingers() {
    for(auto& finger: finger_states_) {
        auto& state = finger.second;
        if(!state.down) {
            continue;
        }

        on_finger_up(finger.first, state.x, state.y);
        if(finger.first == 0) {
            on_mouse_up(
                TOUCH_MOUSE_ID,
                finger.first,
                state.x * float(width()),
                state.y * float(height()),
                true
            );
        }

        state.down = false;
        state.pressure = 0.0f;
    }
}

bool AndroidWindow::_init_window() {
    android_app* aapp = (android_app*) app->platform_state();

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

    const char* extensions = eglQueryString(dpy_, EGL_EXTENSIONS);
    surfaceless_ = extensions && strstr(extensions, "EGL_KHR_surfaceless_context");

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

        if(d >= max_depth) {
            S_DEBUG("Found config: {0}:{1}:{2}:{3} {4}", r, g, b, a, d);
            max_depth = d;
            config_ = cfg;
        }
    }

    config_ = (config_) ? config_ : supported_configs[0];

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

    set_has_context(true); //Mark that we have a valid GL context
    return true;
}

void AndroidWindow::destroy_window() {
    S_DEBUG("Destroying the EGL display");
    if(dpy_ != EGL_NO_DISPLAY) {
        eglTerminate(dpy_);
    }
    dpy_ = EGL_NO_DISPLAY;

    android_app* aapp = (android_app*) smlt::get_app()->platform_state();
    ANativeActivity_finish(aapp->activity);

    /* android_main mustn't return until the activity's been destroyed.
     * Finishing it pauses and stops it first, and native_app_glue blocks
     * Android's main thread on each of those until this thread has seen the
     * command, so if nobody's left to see them, the app stops responding
     * (and Android kills it) */
    while(!aapp->destroyRequested) {
        struct android_poll_source* source = nullptr;
        int events = 0;
        int ident = ALooper_pollOnce(100, nullptr, &events, (void**) &source);
        if(ident >= 0 && source) {
            source->process(aapp, source);
        }
    }
}

static const KeyboardCode KEYCODE_MAPPING[256] = {
    KEYBOARD_CODE_NONE, // AKEYCODE_UNKNOWN
    KEYBOARD_CODE_LEFT,
    KEYBOARD_CODE_RIGHT,
    KEYBOARD_CODE_NONE, // HOME
    KEYBOARD_CODE_AC_BACK, // BACK
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


void AndroidWindow::check_events() {
    android_app* aapp = (android_app*) get_app()->platform_state();

    // Drain every pending event without blocking. ALooper_pollAll() is
    // unavailable in newer NDKs (it can miss wakes); ALooper_pollOnce()
    // returns ALOOPER_POLL_TIMEOUT once nothing is left, or
    // ALOOPER_POLL_CALLBACK if it only ran callbacks (so poll again).
    while(true) {
        struct android_poll_source* source = nullptr;
        int events = 0;
        int ident =
            ALooper_pollOnce(0, nullptr, &events, (void**) &source);

        if(ident == ALOOPER_POLL_CALLBACK) {
            continue;
        }

        if(ident < 0) {
            break;
        }

        if(source != nullptr) {
            source->process(aapp, source);
        }
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

            /* APP_CMD_INIT_WINDOW and APP_CMD_TERM_WINDOW are handled as
             * they arrive, see android_handle_command */
            case APP_CMD_DESTROY:
                set_has_context(false);
                destroy_window();
                break;
            /* The insets change with the layout, e.g. when the device is
             * turned around */
            case APP_CMD_CONFIG_CHANGED:
            case APP_CMD_CONTENT_RECT_CHANGED:
            case APP_CMD_WINDOW_RESIZED:
                update_safe_area();
                break;
            case APP_CMD_GAINED_FOCUS:
                S_INFO("CMD: Focus gained");
                update_safe_area();
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
                // We won't hear about any fingers being lifted now
                release_all_fingers();
               // {
               //     //See Window::context_lock_ for details
               //     thread::Lock<thread::Mutex> context_lock(this->context_lock());
               //     set_has_context(false);
               // }
                break;
            case APP_CMD_PAUSE:
                S_WARN("CMD: PAUSE");
                paused_ = true;
                set_has_focus(false);
                release_all_fingers();
                // Nothing stops the sound playing while we're in the
                // background otherwise
                if(ALCcontext* context = alcGetCurrentContext()) {
                    alcDevicePauseSOFT(alcGetContextsDevice(context));
                }
                break;
            case APP_CMD_RESUME:
                S_WARN("CMD: RESUME");
                paused_ = false;
                set_has_focus(true);
                if(ALCcontext* context = alcGetCurrentContext()) {
                    alcDeviceResumeSOFT(alcGetContextsDevice(context));
                }
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
                    input_state->_handle_key_up(KeyboardID(0), key);
                    on_key_up(key, ModifierKeyState());
                } else if(evt.key.action == AKEY_EVENT_ACTION_DOWN) {
                    input_state->_handle_key_down(KeyboardID(0), key);
                    on_key_down(key, ModifierKeyState());
                } else {}
            } break;
            case AINPUT_EVENT_TYPE_MOTION: {
                auto pointer = evt.touch.pointer_id;
                float x = evt.touch.x;
                float y = evt.touch.y;

                if(evt.touch.action == AMOTION_EVENT_ACTION_DOWN ||
                   evt.touch.action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                    // A finger that never came up (we missed it), so lift it
                    // first rather than leaving it down twice
                    auto& previous = finger_states_[pointer];
                    if(previous.down) {
                        on_finger_up(pointer, previous.x, previous.y);
                        previous.down = false;
                    }

                    on_finger_down(
                        pointer,
                        x,
                        1.0f - y,
                        evt.touch.pressure
                    );

                    if(pointer == 0) {
                        on_mouse_down(
                            TOUCH_MOUSE_ID,
                            pointer,
                            x * float(width()),
                            (1.0f - y) * float(height()),
                            true
                        );
                    }

                    auto& state = finger_states_[pointer];
                    state.x = x;
                    state.y = 1.0f - y;
                    state.pressure = evt.touch.pressure;
                    state.down = true;
                } else if(
                    evt.touch.action == AMOTION_EVENT_ACTION_POINTER_UP ||
                    evt.touch.action == AMOTION_EVENT_ACTION_UP ||
                    evt.touch.action == AMOTION_EVENT_ACTION_CANCEL) {

                    auto& state = finger_states_[pointer];

                    // This avoids a double-release if we get ACTION_UP and
                    // then CANCEL. (It's not the pressure that's checked,
                    // as some touch screens always report 0.)
                    if(state.down) {
                        on_finger_up(pointer, x, 1.0f - y);

                        if(pointer == 0) {
                            on_mouse_up(
                                TOUCH_MOUSE_ID,
                                pointer,
                                x * float(width()),
                                (1.0f - y) * float(height()),
                                true
                            );
                        }

                        state.x = x;
                        state.y = 1.0f - y;
                        state.pressure = 0.0f;
                        state.down = false;
                    }
                } else if(evt.touch.action == AMOTION_EVENT_ACTION_MOVE) {
                    auto& state = finger_states_[pointer];
                    if(!state.down) {
                        break;
                    }

                    float dx = x - state.x;
                    float dy = (1.0f - y) - state.y;

                    on_finger_motion(pointer, x, 1.0f - y, dx, dy);

                    if(pointer == 0) {
                        on_mouse_move(
                            TOUCH_MOUSE_ID,
                            x * float(width()),
                            (1.0f - y) * float(height()),
                            true
                        );
                    }

                    state.x = x;
                    state.y = 1.0f - y;
                    state.pressure = evt.touch.pressure;
                } else {
                    S_ERROR("ACTION_UNKNOWN ({0}) {1}", evt.touch.action, pointer);
                }
            } break;

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
    const char* from_env = std::getenv("SIMULANT_SOUND_DRIVER");

    std::string selected = (from_env) ? from_env :
                               (from_config.empty()) ? "openal" : from_config;

    if(selected == "null") {
        S_DEBUG("Null sound driver activated");
        return std::make_shared<NullSoundDriver>(this);
    } else {
        if(selected != "openal") {
            S_WARN("Unknown sound driver ({0}) falling back to OpenAL", selected);
        }
        S_DEBUG("OpenAL sound driver activated");
        return std::make_shared<OpenALSoundDriver>(this);
    }
}

}
