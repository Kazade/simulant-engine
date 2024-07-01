#pragma once

#include "../../keycodes.h"
#include "simulant/nodes/stage_node.h"
#include "widget.h"

namespace smlt {

class EventListener;

namespace ui {

/** An onscreen keyboard.
 */

class Frame;
class TextEntry;

struct SoftKeyPressedEvent {
    KeyboardCode code;
    char16_t chr;

    bool cancelled = false;

    void cancel() {
        cancelled = true;
    }
};

typedef sig::signal<void(SoftKeyPressedEvent&)> KeyboardKeyPressedSignal;
typedef sig::signal<void(const unicode&)> KeyboardDoneSignal;
typedef sig::signal<void()> KeyboardCancelledSignal;

enum KeyboardMode {
    KEYBOARD_MODE_UPPERCASE,
    KEYBOARD_MODE_LOWERCASE,
    KEYBOARD_MODE_NUMERICAL,
    KEYBOARD_MODE_ACCENT_LOWERCASE,
    KEYBOARD_MODE_ACCENT_UPPERCASE
};

class KeyboardPanel;

/* A keyboard is combined of a TextInput and a KeyboardPanel */
class Keyboard: public Widget {

    DEFINE_SIGNAL(KeyboardKeyPressedSignal, signal_key_pressed);
    DEFINE_SIGNAL(KeyboardDoneSignal, signal_done);
    DEFINE_SIGNAL(KeyboardCancelledSignal, signal_cancelled);

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_WIDGET_KEYBOARD);

    S_DEFINE_STAGE_NODE_PARAM(Keyboard, "mode", int,
                              int(KEYBOARD_MODE_UPPERCASE),
                              "The mode of the keyboard");
    S_DEFINE_CORE_WIDGET_PROPERTIES(Keyboard);

    using Widget::clean_up;
    using Widget::init; // Pull in init to satisfy TwoPhaseConstructed<Keyboard>

    Keyboard(Scene* owner);
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

    TextEntry* entry() {
        return entry_;
    }

private:
    bool on_create(Params params) override;
    void on_transformation_change_attempted() override;

    UIDim calculate_content_dimensions(Px text_width, Px text_height) override;

    KeyboardPanel* panel_ = nullptr;
    TextEntry* entry_ = nullptr;
    Frame* info_row_ = nullptr;

    Frame* main_frame_ = nullptr;

    std::shared_ptr<EventListener> keyboard_listener_;

    bool pre_set_text(const unicode& text) override;

    const unicode& calc_text() const override;
};

} // namespace ui

} // namespace smlt
