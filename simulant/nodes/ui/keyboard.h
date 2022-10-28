#pragma once

#include <map>
#include "widget.h"
#include "../../keycodes.h"

namespace smlt {

class EventListener;

namespace ui {

/** An onscreen keyboard.
 */

class Frame;

enum KeyboardLayout {
    KEYBOARD_LAYOUT_ALPHABETICAL,
    KEYBOARD_LAYOUT_NUMERICAL,
};

struct SoftKeyPressedEvent {
    KeyboardCode code;
    wchar_t character;

    bool cancelled = false;

    void cancel() {
        cancelled = true;
    }
};

typedef sig::signal<void (SoftKeyPressedEvent&)> KeyboardKeyPressedSignal;
typedef sig::signal<void ()> KeyboardDoneSignal;

enum KeyboardMode {
    KEYBOARD_MODE_UPPERCASE,
    KEYBOARD_MODE_LOWERCASE
};

class KeyboardPanel;

/* A keyboard is combined of a TextInput and a KeyboardPanel */
class Keyboard:
    public Widget {

    DEFINE_SIGNAL(KeyboardKeyPressedSignal, signal_key_pressed);
    DEFINE_SIGNAL(KeyboardDoneSignal, signal_done);
public:
    using Widget::init; // Pull in init to satisfy TwoPhaseConstructed<Keyboard>
    using Widget::clean_up;

    Keyboard(UIManager* owner, UIConfig* config, Stage *stage, KeyboardLayout layout);
    ~Keyboard();

    void cursor_up();
    void cursor_down();
    void cursor_right();
    void cursor_left();

    /** Sends the key_pressed signal for the selected key */
    void activate();
    void set_mode(KeyboardMode mode);

    bool is_keyboard_integration_enabled() const {
        return bool(keyboard_listener_);
    }

    void set_keyboard_integration_enabled(bool value);

    using Widget::set_font;

    void set_font(FontPtr font) override;
private:

    KeyboardLayout layout_ = KEYBOARD_LAYOUT_ALPHABETICAL;
    KeyboardMode mode_ = KEYBOARD_MODE_UPPERCASE;

    std::shared_ptr<KeyboardPanel> panel_;

    Frame* main_frame_ = nullptr;

    std::shared_ptr<EventListener> keyboard_listener_;
};

}
}
