#include "kazbase/list_utils.h"
#include "kazbase/unicode.h"
#include "kazlog/kazlog.h"
#include "input_controller.h"
#include "window_base.h"
#include "virtual_gamepad.h"

namespace kglt {

InputConnection::InputConnection(InputConnectionID id, std::weak_ptr<Device> device):
    generic::Identifiable<InputConnectionID>(id),
    device_(device) {

}

InputConnection Device::new_input_connection() {
    static uint32_t idx = 0;
    return InputConnection(InputConnectionID(++idx), shared_from_this());
}

void InputConnection::disconnect() {
    auto device = device_.lock();
    if(device) {
        device->disconnect(*this);
    }
}

InputConnection Keyboard::key_pressed_connect(GlobalKeyCallback callback) {
    InputConnection c = new_input_connection();

    if(global_key_press_signals_.find(c) != global_key_press_signals_.end()) {
        throw ValueError("Something went wrong when generating the connection ID");
    }

    global_key_press_signals_[c] = callback;
    return c;
}

InputConnection Keyboard::key_pressed_connect(SDL_Scancode code, KeyCallback callback) {
    InputConnection c = new_input_connection();    
    key_press_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::key_while_pressed_connect(GlobalKeyDownCallback callback) {
    InputConnection c = new_input_connection();

    if(global_while_key_pressed_signals_.find(c) != global_while_key_pressed_signals_.end()) {
        throw ValueError("Something went wrong when generating the connection ID");
    }

    global_while_key_pressed_signals_[c] = callback;
    return c;
}

InputConnection Keyboard::key_while_pressed_connect(SDL_Scancode code, KeyDownCallback callback) {
    InputConnection c = new_input_connection();
    key_while_down_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::key_released_connect(GlobalKeyCallback callback) {
    InputConnection c = new_input_connection();
    global_key_release_signals_[c] = callback;
    return c;
}

InputConnection Keyboard::key_released_connect(SDL_Scancode code, KeyCallback callback) {
    InputConnection c = new_input_connection();
    key_release_signals_[code][c] = callback;
    return c;
}

InputConnection Keyboard::text_input_connect(TextInputCallback callback) {
    InputConnection c = new_input_connection();
    text_input_signals_[c] = callback;
    return c;
}

Keyboard::Keyboard() {

}

void Keyboard::_handle_keydown_event(SDL_Keysym key) {
    bool propagation_stopped = false;

    //First trigger all global handlers
    for(GlobalKeySignalEntry entry: global_key_press_signals_) {
        if(entry.second(key)) {
            propagation_stopped = true;
        }
    }

    //If a global handler returned true, we don't trigger any more
    //signals
    if(!propagation_stopped) {
        if(container::contains(key_press_signals_, key.scancode)) {
            for(KeySignalEntry entry: key_press_signals_[key.scancode]) {
                entry.second(key);
            }
        }

        //This is inside the IF because, otherwise while_key_pressed events will
        //trigger and we don't want that if the key down was handled... feels dirty
        state_[key.scancode] = true;
        keys_down_[key.scancode] = key;
    }
}

bool modifier_is_set(uint32_t state, SDL_Keymod modifier) {
    return (state & modifier) == modifier;
}

void Keyboard::_handle_text_input_event(SDL_TextInputEvent key) {
    for(auto entry: text_input_signals_) {
        entry.second(key);
    }
}

void Keyboard::_handle_keyup_event(SDL_Keysym key) {
    bool propagation_stopped = false;

    //First trigger all global handlers
    for(GlobalKeySignalEntry entry: global_key_release_signals_) {
        if(entry.second(key)) {
            propagation_stopped = true;
        }
    }

    if(!propagation_stopped) {
        if(container::contains(key_release_signals_, key.scancode)) {
            for(KeySignalEntry entry: key_release_signals_[key.scancode]) {
                entry.second(key);
            }
        }
        state_[key.scancode] = false;
        keys_down_.erase(key.scancode);
    }

}

void Keyboard::_update(double dt) {
    for(auto keysym: keys_down_) {
        for(auto conn: global_while_key_pressed_signals_) {
            conn.second(keysym.second, dt);
        }
    }

    for(auto p: key_while_down_signals_) {
        if(key_state(p.first)) {
            for(std::pair<InputConnection, KeyDownCallback> p2: p.second) {
                p2.second(keys_down_[p.first], dt);
            }
        }
    }
}

void Keyboard::_disconnect(const InputConnection &connection) {
    for(auto p: key_press_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }

    for(auto p: key_while_down_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }

    for(auto p: key_release_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }

    text_input_signals_.erase(connection);
    global_key_press_signals_.erase(connection);
}

Joypad::Joypad():
    jitter_value_(3) {

}

InputConnection Joypad::axis_changed_connect(JoypadAxis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_changed_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_nonzero_connect(JoypadAxis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_while_nonzero_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_below_zero_connect(JoypadAxis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_while_below_zero_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::axis_while_above_zero_connect(JoypadAxis axis, JoypadCallback callback) {
    InputConnection c = new_input_connection();
    axis_while_above_zero_signals_[axis][c] = callback;
    return c;
}

InputConnection Joypad::button_pressed_connect(Button button, JoypadButtonCallback callback) {
    InputConnection c = new_input_connection();
    button_pressed_signals_[button][c] = callback;
    return c;
}

InputConnection Joypad::button_released_connect(Button button, JoypadButtonCallback callback) {
    InputConnection c = new_input_connection();
    button_released_signals_[button][c] = callback;
    return c;
}

InputConnection Joypad::button_while_down_connect(Button button, JoypadButtonDownCallback callback) {
    InputConnection c = new_input_connection();
    button_down_signals_[button][c] = callback;
    return c;
}

InputConnection Joypad::hat_changed_connect(Hat hat, JoypadHatCallback callback) {
    InputConnection c = new_input_connection();
    hat_changed_signals_[hat][c] = callback;
    return c;
}

InputConnection Joypad::hat_while_not_centered_connect(Hat hat, JoypadHatDownCallback callback) {
    InputConnection c = new_input_connection();
    hat_while_not_centered_signals_[hat][c] = callback;
    return c;
}

void Joypad::_handle_button_down_event(Button button) {
    if(container::contains(button_pressed_signals_, button)) {
        for(ButtonSignalEntry entry: button_pressed_signals_[button]) {
            entry.second(button);
        }
    }

    button_state_[button] = true;
}

void Joypad::_handle_button_up_event(Button button) {
    if(container::contains(button_released_signals_, button)) {
        for(ButtonSignalEntry entry: button_released_signals_[button]) {
            entry.second(button);
        }
    }

    button_state_[button] = false;
}

void Joypad::_handle_axis_changed_event(JoypadAxis axis, int32_t value) {
    if(container::contains(axis_changed_signals_, axis)) {
        for(AxisSignalEntry entry: axis_changed_signals_[axis]) {
            entry.second(float(value) / float(32768), axis);
        }
    }

    axis_state_[axis] = value;
}

void Joypad::_handle_hat_changed_event(Hat hat, HatPosition position) {
    if(container::contains(hat_changed_signals_, hat)) {
        for(HatSignalEntry entry: hat_changed_signals_[hat]) {
            entry.second(position, hat);
        }
    }

    hat_state_[hat] = position;
}

void Joypad::_update(double dt) {
    for(std::pair<JoypadAxis, int32_t> p: axis_state_) {
        JoypadAxis axis = p.first;
        int32_t axis_state = p.second;

        if(axis_state > jitter_value_ || axis_state < -jitter_value_) {
            for(AxisSignalEntry entry: axis_while_nonzero_signals_[axis]) {
                entry.second(float(axis_state) / float(32768), axis);
            }
        }
        if(axis_state > jitter_value_) {
            for(AxisSignalEntry entry: axis_while_above_zero_signals_[axis]) {
                entry.second(float(axis_state) / float(32768), axis);
            }
        }

        if(axis_state < -jitter_value_) {
            for(AxisSignalEntry entry: axis_while_below_zero_signals_[axis]) {
                entry.second(float(axis_state) / float(32768), axis);
            }
        }
    }

    for(std::pair<Hat, HatPosition> p: hat_state_) {
        HatPosition pos = p.second;
        if(p.second != HAT_POSITION_CENTERED) {
            for(auto entry: hat_while_not_centered_signals_[p.first]) {
                entry.second(pos, p.first, dt);
            }
        }
    }

    for(auto state: button_state_) {
        if(state.second) {
            for(auto signal: button_down_signals_[state.first]) {
                signal.second(state.first, dt);
            }
        }
    }
}

void Joypad::_disconnect(const InputConnection &connection) {
    for(std::pair<JoypadAxis, std::map<InputConnection, JoypadCallback> > p: axis_changed_signals_) {
        if(container::contains(p.second, connection)) {
            p.second.erase(connection);
        }
    }
}

InputController::InputController(WindowBase& window):
    window_(window),
    keyboard_(new Keyboard()),
    mouse_(new Mouse()) {

    //SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_JoystickEventState(SDL_ENABLE);
    for(uint8_t i = 0; i < SDL_NumJoysticks(); ++i) {
        if(SDL_IsGameController(i)) {
            joypads_.push_back(Joypad::create());
            sdl_joysticks_.push_back(SDL_GameControllerOpen(i));
        }
    }
}

InputController::~InputController() {
    //Disable joystick events
    SDL_JoystickEventState(SDL_DISABLE);

    //Make sure we close the joysticks   
    if(SDL_WasInit(SDL_INIT_JOYSTICK)) {
        for(auto joy: sdl_joysticks_) {
            if(joy) {
                SDL_GameControllerClose(joy);
            }
        }
    }
}

auto SDL_axis_to_KGLT_axis(Uint8 axis) {
    switch(axis) {
    case SDL_CONTROLLER_AXIS_LEFTX: return JOYPAD_AXIS_LEFT_X;
    case SDL_CONTROLLER_AXIS_LEFTY: return JOYPAD_AXIS_LEFT_Y;
    case SDL_CONTROLLER_AXIS_RIGHTX: return JOYPAD_AXIS_RIGHT_X;
    case SDL_CONTROLLER_AXIS_RIGHTY: return JOYPAD_AXIS_RIGHT_Y;
    default:
        throw NotImplementedError(__FILE__, __LINE__);
    }
}

void InputController::handle_event(SDL_Event &event) {
    switch(event.type) {
        case SDL_KEYDOWN:
            keyboard()._handle_keydown_event(event.key.keysym);
        break;
        case SDL_KEYUP:
            keyboard()._handle_keyup_event(event.key.keysym);
        break;
        case SDL_MOUSEMOTION:
            mouse()._handle_motion_event(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
        break;
        case SDL_TEXTINPUT:
            keyboard()._handle_text_input_event(event.text);
        break;
        case SDL_JOYAXISMOTION:
            joypad(event.jaxis.which)._handle_axis_changed_event(
                SDL_axis_to_KGLT_axis(event.jaxis.axis), event.jaxis.value
            );
        break;
        case SDL_JOYBUTTONDOWN:
            joypad(event.jbutton.which)._handle_button_down_event(event.jbutton.button);
        break;
        case SDL_JOYBUTTONUP:
            joypad(event.jbutton.which)._handle_button_up_event(event.jbutton.button);
        break;
        case SDL_JOYHATMOTION:
            joypad(event.jhat.which)._handle_hat_changed_event(event.jhat.hat, (HatPosition)event.jhat.value);
        break;
        default:
            break;
    }
}

void InputController::update(double dt) {
    keyboard_->_update(dt);
    mouse_->_update(dt);

    for(Joypad::ptr j: joypads_) {
        j->_update(dt);
    }

    if(virtual_joypad_) {
        virtual_joypad_->_update(dt);
    }
}

void InputController::init_virtual_joypad() {
    if(!virtual_joypad_) {
        virtual_joypad_ = std::make_shared<Joypad>();
    }

    auto& vpad = window_.virtual_joypad;

    for(auto conn: virtual_joypad_connections_) {
        conn.disconnect();
    }
    virtual_joypad_connections_.clear();

    virtual_joypad_connections_.push_back(
        vpad->signal_button_down().connect([=](int btn) {
            joypad(joypads_.size())._handle_button_down_event(btn);
        })
    );

    virtual_joypad_connections_.push_back(
        vpad->signal_button_up().connect([=](int btn) {
            joypad(joypads_.size())._handle_button_up_event(btn);
        })
    );

    virtual_joypad_connections_.push_back(
        vpad->signal_hat_changed().connect([=](HatPosition pos) {
            joypad(joypads_.size())._handle_hat_changed_event(0, pos);
        })
    );
}

Joypad& InputController::joypad(uint8_t idx) {
    if(idx == joypads_.size() && window_.has_virtual_joypad()) {
        init_virtual_joypad();
        return *virtual_joypad_;
    }
    return *joypads_.at(idx);
}

uint8_t InputController::joypad_count() const {
    auto ret = joypads_.size();
    if(window_.has_virtual_joypad()) {
        ++ret;
    }

    return ret;
}

InputConnection Mouse::motion_event_connect(MouseMotionCallback callback) {
    InputConnection c = new_input_connection();
    motion_event_signals_[c] = callback;
    return c;
}

void Mouse::_handle_motion_event(int32_t x, int32_t y, int32_t relx, int32_t rely) {
    last_motion_event_ = MotionEvent(x, y, relx, rely);
}

void Mouse::_disconnect(const InputConnection &connection) {
    motion_event_signals_.erase(connection);
}

void Mouse::_update(double dt) {
    for(auto& entry: this->motion_event_signals_) {
        entry.second(
            last_motion_event_.x,
            last_motion_event_.y,
            last_motion_event_.relx,
            last_motion_event_.rely
        );
    }

    last_motion_event_ = MotionEvent(); //Reset event
}


}
