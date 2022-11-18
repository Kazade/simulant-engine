#pragma once

#include <vector>
#include <functional>
#include <map>
#include <unordered_map>
#include "../generic/property.h"
#include "../generic/managed.h"
#include "../keycodes.h"
#include "../signals/signal.h"
#include "../event_listener.h"

namespace smlt {

class SceneBase;
class InputState;
class InputAxis;
class GameController;

namespace ui {
    class Keyboard;
}

typedef std::function<void (InputAxis*)> EachAxisCallback;
typedef std::vector<InputAxis*> AxisList;

struct TextInputReceivedControl {
    friend class InputManager;

    void cancel() {
        cancelled = true;
    }

private:
    bool cancelled = false;
};

typedef sig::signal<bool (const char16_t&, TextInputReceivedControl&)> TextInputReceivedSignal;

enum JoystickButton : int8_t;

class InputManager:
    public RefCounted<InputManager> {

    /** Signal for receiving text input when text input mode is active.
     *  Returning false from this callback will cause the entered character
     *  to be ignored. The built-in on-screen keyboard will reflect that
     *  visually */
    DEFINE_SIGNAL(TextInputReceivedSignal, signal_text_input_received);

public:
    InputManager(InputState* controller);
    ~InputManager();

    InputAxis* new_axis(const std::string& name);
    AxisList axises(const std::string& name);
    void each_axis(EachAxisCallback callback);
    void destroy_axises(const std::string& name);
    void destroy_axis(InputAxis* axis);
    std::size_t axis_count(const std::string& name) const;

    float axis_value(const std::string& name) const;
    int8_t axis_value_hard(const std::string& name) const;

    void update(float dt);

    /* Returns true if the axis was just pressed this frame */
    bool axis_was_pressed(const std::string& name) const;

    /* Returns true if the axis was just released this frame */
    bool axis_was_released(const std::string& name) const;

    /** Starts reading text input from the user. This will return true
     * if an onscreen keyboard was displayed, and false if it wasn't. An
     * onscreen keyboard will be displayed if no physical keyboard is present */
    bool start_text_input(bool force_onscreen=false);

    /** Stops gathering user input. If an onscreen keyboard is visible, this will
     * hide it and return the text that was entered. */
    unicode stop_text_input();

    /** Returns true if the text input is currently active. While text input is
     * active the signal_text_input_received signal will receive characters that
     * are entered */
    bool text_input_active() const {
        return text_input_enabled_;
    }

    bool onscreen_keyboard_active() const {
        return bool(keyboard_);
    }

private:
    InputState* controller_;

    std::multimap<std::string, std::shared_ptr<InputAxis>> axises_;

    std::unordered_map<std::string, bool> prev_axis_states_;
    std::unordered_map<std::string, bool> axis_states_;

    float _calculate_value(InputAxis* axis) const;

    bool _update_keyboard_axis(InputAxis* axis, float dt);
    bool _update_mouse_button_axis(InputAxis* axis, float dt);
    bool _update_joystick_button_axis(InputAxis* axis, float dt);
    void _update_mouse_axis_axis(InputAxis *axis, float dt);
    bool _update_joystick_axis_axis(InputAxis* axis, float dt);
    bool _update_joystick_hat_axis(InputAxis* axis, float dt);

    void _process_mouse(int8_t id, int8_t pbtn, int8_t nbtn, bool* positive_pressed, bool* negative_pressed);
    void _process_game_controller(GameController* controller, JoystickButton pbtn, JoystickButton nbtn, bool *positive_pressed, bool *negative_pressed);
    void _process_keyboard(int8_t id, KeyboardCode pbtn, KeyboardCode nbtn, bool *positive_pressed, bool *negative_pressed);

    bool text_input_enabled_ = false;


    /* Watch for keyboard inputs from the window */
    class TextInputHandler : public EventListener {
    public:
        TextInputHandler(InputManager* self): self_(self) {}

        void on_key_down(const KeyEvent& evt) override;
    private:
        InputManager* self_;
    };

    TextInputHandler event_listener_ = {this};

    StagePtr keyboard_stage_;
    CameraPtr keyboard_camera_;
    PipelinePtr keyboard_pipeline_;
    ui::Keyboard* keyboard_ = nullptr;

    sig::connection scene_deactivated_conn_;
    void on_scene_deactivated(std::string, SceneBase* scene);

public:
    S_DEFINE_PROPERTY(state, &InputManager::controller_);
    S_DEFINE_PROPERTY(onscreen_keyboard, &InputManager::controller_);
};

}
