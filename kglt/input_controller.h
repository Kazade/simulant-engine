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

typedef std::function<void (KeyCode)> KeyboardCallback;

class Keyboard :
    public Device,
    public Managed<Keyboard> {

public:
    Keyboard();
    InputConnection key_pressed_connect(KeyCode code, KeyboardCallback callback);
    InputConnection key_while_down_connect(KeyCode code, KeyboardCallback callback);
    InputConnection key_released_connect(KeyCode code, KeyboardCallback callback);

private:
    typedef std::pair<InputConnection, KeyboardCallback> SignalEntry;

    void _handle_keydown_event(KeyCode key);
    void _handle_keyup_event(KeyCode key);
    void _update();
    void _disconnect(const InputConnection &connection);

    std::vector<uint8_t> state_;

    std::map<KeyCode, std::map<InputConnection, KeyboardCallback> > key_press_signals_;
    std::map<KeyCode, std::map<InputConnection, KeyboardCallback> > key_while_down_signals_;
    std::map<KeyCode, std::map<InputConnection, KeyboardCallback> > key_release_signals_;

    friend class InputController;
};

class Mouse :
    public Device,
    public Managed<Mouse> {

};

class Joypad :
    public Device,
    public Managed<Joypad> {
public:
    InputConnection axis_changed_connect(uint8_t axis, std::function<bool (uint8_t, uint32_t)> callback);
    InputConnection axis_while_nonzero_connect(uint8_t axis, std::function<bool (uint8_t, uint32_t)> callback);
};

class InputController:
    public Managed<InputController> {

public:
    InputController();

    Keyboard& keyboard() { assert(keyboard_); return *keyboard_; }
    Mouse& mouse() { assert(mouse_); return *mouse_; }
    Joypad& joypad(uint8_t idx=0) { return *joypads_.at(idx); }
    uint8_t joypad_count() const { return joypads_.size(); }

    void update();
    void handle_event(SDL_Event& event);

private:
    Keyboard::ptr keyboard_;
    Mouse::ptr mouse_;
    std::vector<Joypad::ptr> joypads_;
};

}
#endif // INPUT_CONTROLLER_H
