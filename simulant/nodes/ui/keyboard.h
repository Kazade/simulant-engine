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


struct SoftKeyPressedEvent {
    KeyboardCode code;
    wchar_t character;

    bool cancelled = false;

    void cancel() {
        cancelled = true;
    }
};

typedef sig::signal<void (SoftKeyPressedEvent&)> KeyboardKeyPressedSignal;
typedef sig::signal<void (const unicode&)> KeyboardDoneSignal;
typedef sig::signal<void ()> KeyboardCancelledSignal;

enum KeyboardMode {
    KEYBOARD_MODE_UPPERCASE,
    KEYBOARD_MODE_LOWERCASE,
    KEYBOARD_MODE_NUMERICAL,
    KEYBOARD_MODE_ACCENT_LOWERCASE,
    KEYBOARD_MODE_ACCENT_UPPERCASE
};

class KeyboardPanel;

/* A keyboard is combined of a TextInput and a KeyboardPanel */
class Keyboard:
    public Widget {

    DEFINE_SIGNAL(KeyboardKeyPressedSignal, signal_key_pressed);
    DEFINE_SIGNAL(KeyboardDoneSignal, signal_done);
    DEFINE_SIGNAL(KeyboardCancelledSignal, signal_cancelled);
public:
    using Widget::init; // Pull in init to satisfy TwoPhaseConstructed<Keyboard>
    using Widget::clean_up;

    Keyboard(UIManager* owner, UIConfig* config, Stage *stage, KeyboardMode mode, const unicode& initial_text="");
    ~Keyboard();

    void cursor_up();
    void cursor_down();
    void cursor_right();
    void cursor_left();
    bool cursor_to_char(uint16_t displayed_char);
    void cursor_to_return();
    void cursor_to_case_toggle();
    void cursor_to_backspace();
    void cursor_to_ok();
    void cursor_to_space();

    /** Sends the key_pressed signal for the selected key */
    void activate();
    void cancel();

    void set_mode(KeyboardMode mode);
    KeyboardMode mode() const;

    bool is_keyboard_integration_enabled() const {
        return bool(keyboard_listener_);
    }

    void set_keyboard_integration_enabled(bool value);

    using Widget::set_font;

    void set_font(FontPtr font) override;
private:
    void on_transformation_change_attempted() override;

    UIDim calculate_content_dimensions(Px text_width, Px text_height) override;

    std::shared_ptr<KeyboardPanel> panel_;
    std::shared_ptr<Label> entry_;
    std::shared_ptr<Frame> info_row_;

    Frame* main_frame_ = nullptr;

    std::shared_ptr<EventListener> keyboard_listener_;

    bool pre_set_text(const unicode& text) override;
};

}
}
