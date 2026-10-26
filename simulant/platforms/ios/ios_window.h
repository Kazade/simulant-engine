#pragma once

#include <map>
#include <vector>

#include "../../window.h"

namespace smlt {

/* Working with iOS is a bit different to other platforms. UIKit has to own
 * the main thread from the start, so main() hands over to this, which starts
 * UIKit and then calls `app_main` once the app has a window. Your main()
 * should look something like:
 *
 *     int game_main(int argc, char* argv[]) {
 *         AppConfig config;
 *         MyApp app(config);
 *         return app.run();
 *     }
 *
 *     int main(int argc, char* argv[]) {
 *         return smlt::ios_main(argc, argv, game_main);
 *     }
 *
 * This never returns: when `app_main` does, the process exits with its
 * result. The app's Info.plist needs a UIApplicationSceneManifest, as the
 * window is created for a scene (see platforms/ios/Info.plist.in). */
int ios_main(int argc, char* argv[], int (*app_main)(int, char*[]));

class IOSWindow : public Window {
public:
    static Window::ptr create(Application* app) {
        return Window::create<IOSWindow>(app);
    }

    IOSWindow();

    void set_title(const std::string&) override {} // No-op
    void cursor_position(int32_t&, int32_t&) override {} // No-op
    void show_cursor(bool) override {} // No-op
    void lock_cursor(bool) override {} // No-op

    void destroy_window() override;
    void check_events() override;

    void initialize_input_controller(InputState &controller) override;

    std::shared_ptr<SoundDriver> create_sound_driver(const std::string& from_config) override;

    /* Nothing can be drawn while the app is in the background: iOS kills
     * apps that use the GPU then */
    bool can_render() const override { return framebuffer_ != 0 && !in_background_; }

    /* Plays on the Taptic Engine, where the device has one */
    bool has_haptics() const override;
    void start_haptics(NormalizedFloat strength, const Seconds& duration) override;
    void stop_haptics() override;

    /* These are called from UIKit (see ios_window.mm), as things happen */
    enum TouchPhase {
        TOUCH_PHASE_BEGAN,
        TOUCH_PHASE_MOVED,
        TOUCH_PHASE_ENDED
    };

    void on_touch(const void* touch, TouchPhase phase, float x, float y, float pressure);
    void on_key(int hid_usage, bool down);
    void on_layout_changed();
    void on_active_changed(bool active);
    void on_background_changed(bool background);

private:
    void do_swap_buffers() override;

    bool _init_window() override;
    bool _init_renderer(Renderer *renderer) override;

    /* (Re)allocates the renderbuffers for the view's current size */
    bool create_framebuffer();
    void destroy_framebuffer();

    /* Reads the safe area from the view */
    void update_safe_area();

    /* Lifts every finger that's down, e.g. when the app loses focus and
     * won't hear about them being lifted */
    void release_all_fingers();

    /* The EAGLContext, and the CoreHaptics engine and player, retained by
     * us (they're Objective-C objects) */
    void* context_ = nullptr;
    mutable void* haptic_engine_ = nullptr;
    void* haptic_player_ = nullptr;
    mutable bool haptics_checked_ = false;

    /* There's no default framebuffer on iOS, so this is what's drawn to */
    uint32_t framebuffer_ = 0;
    uint32_t color_renderbuffer_ = 0;
    uint32_t depth_renderbuffer_ = 0;

    bool layout_changed_ = false;
    bool in_background_ = false;

    struct TouchEvent {
        int finger_id;
        TouchPhase phase;
        float x;
        float y;
        float pressure;
    };

    struct KeyEvent {
        int hid_usage;
        bool down;
    };

    /* Events arrive while check_events() runs UIKit's run loop, and are
     * handled once it's done */
    std::vector<TouchEvent> touch_events_;
    std::vector<KeyEvent> key_events_;

    /* UIKit has no numbers for touches, so each is given the lowest one
     * that's free (the first finger is always 0, as on other platforms) */
    std::map<const void*, int> finger_ids_;

    struct FingerState {
        float x = 0.0f;
        float y = 0.0f;
        float pressure = 0.0f;
        bool down = false;
    };

    std::map<int, FingerState> finger_states_;
};

}
