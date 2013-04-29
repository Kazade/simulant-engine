#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include <cstdint>
#include <vector>
#include <map>
#include <tr1/functional>
#include <SDL/SDL.h>

#include "generic/managed.h"
#include "generic/identifiable.h"
#include "generic/unique_id.h"
#include "keyboard.h"

namespace kglt {

typedef UniqueID<300> InputConnectionID;

class InputController;
class InputConnection;

class Device {
public:
    void disconnect(const InputConnection& connection) {
        _disconnect(connection);
    }

protected:
    InputConnection new_input_connection();

private:
    virtual void _disconnect(const InputConnection& connection) = 0;

    friend class InputController;
};

class InputConnection :
    public generic::Identifiable<InputConnectionID> {

public:
    InputConnection(InputConnectionID id, Device& device);
    void disconnect();

private:    
    Device& device_;
};

typedef std::function<void (KeyCode)> KeyCallback;
typedef std::function<void (KeyCode, double)> KeyDownCallback;

class Keyboard :
    public Device,
    public Managed<Keyboard> {

public:
    Keyboard();
    InputConnection key_pressed_connect(KeyCallback callback);
    InputConnection key_pressed_connect(KeyCode code, KeyCallback callback);
    InputConnection key_while_down_connect(KeyCode code, KeyDownCallback callback);
    InputConnection key_released_connect(KeyCode code, KeyCallback callback);

private:
    typedef std::pair<InputConnection, KeyCallback> KeySignalEntry;
    typedef std::pair<InputConnection, KeyDownCallback> KeyDownSignalEntry;

    void _handle_keydown_event(KeyCode key);
    void _handle_keyup_event(KeyCode key);
    void _update(double dt);
    void _disconnect(const InputConnection &connection);

    std::vector<uint8_t> state_;

    std::map<InputConnection, KeyCallback> global_key_press_signals_;

    std::map<KeyCode, std::map<InputConnection, KeyCallback> > key_press_signals_;
    std::map<KeyCode, std::map<InputConnection, KeyDownCallback> > key_while_down_signals_;
    std::map<KeyCode, std::map<InputConnection, KeyCallback> > key_release_signals_;

    friend class InputController;
};

class Mouse :
    public Device,
    public Managed<Mouse> {

};

typedef uint8_t Axis;
typedef uint8_t Button;
typedef float AxisRange;

typedef std::function<void (AxisRange, Axis)> JoypadCallback;
typedef std::function<void (uint8_t)> JoypadButtonCallback;

class Joypad :
    public Device,
    public Managed<Joypad> {
public:
    Joypad();

    InputConnection axis_changed_connect(Axis axis, JoypadCallback callback);
    InputConnection axis_while_nonzero_connect(Axis axis, JoypadCallback callback);
    InputConnection axis_while_below_zero_connect(Axis axis, JoypadCallback callback);
    InputConnection axis_while_above_zero_connect(Axis axis, JoypadCallback callback);

    InputConnection button_pressed_connect(Button button, JoypadButtonCallback callback);
    InputConnection button_released_connect(Button button, JoypadButtonCallback callback);

private:
    typedef std::pair<InputConnection, JoypadCallback> AxisSignalEntry;
    typedef std::pair<InputConnection, JoypadButtonCallback> ButtonSignalEntry;

    void _handle_axis_changed_event(Axis axis, int32_t value);
    void _handle_button_down_event(Button button);
    void _handle_button_up_event(Button button);

    void _update();
    void _disconnect(const InputConnection &connection);

    uint8_t jitter_value_;

    std::map<Axis, int32_t> axis_state_;

    std::map<Axis, std::map<InputConnection, JoypadCallback> > axis_changed_signals_;
    std::map<Axis, std::map<InputConnection, JoypadCallback> > axis_while_nonzero_signals_;
    std::map<Axis, std::map<InputConnection, JoypadCallback> > axis_while_below_zero_signals_;
    std::map<Axis, std::map<InputConnection, JoypadCallback> > axis_while_above_zero_signals_;

    std::map<Button, std::map<InputConnection, JoypadButtonCallback> > button_pressed_signals_;
    std::map<Button, std::map<InputConnection, JoypadButtonCallback> > button_released_signals_;

    friend class InputController;
};

class InputController:
    public Managed<InputController> {

public:
    InputController();
    ~InputController();

    Keyboard& keyboard() { assert(keyboard_); return *keyboard_; }
    Mouse& mouse() { assert(mouse_); return *mouse_; }
    Joypad& joypad(uint8_t idx=0) { return *joypads_.at(idx); }
    uint8_t joypad_count() const { return joypads_.size(); }

    void update(double dt);
    void handle_event(SDL_Event& event);

private:
    Keyboard::ptr keyboard_;
    Mouse::ptr mouse_;

    std::vector<Joypad::ptr> joypads_;
    std::vector<SDL_Joystick*> sdl_joysticks_;
};

}
#endif // INPUT_CONTROLLER_H
