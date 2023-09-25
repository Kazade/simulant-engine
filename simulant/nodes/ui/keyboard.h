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
class TextEntry;

struct SoftKeyPressedEvent {
    KeyboardCode code;
    char16_t chr;

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

struct KeyboardParams : public WidgetParams {
    KeyboardMode mode = KEYBOARD_MODE_UPPERCASE;
    unicode initial_text = "";

    KeyboardParams(
        KeyboardMode mode=KEYBOARD_MODE_UPPERCASE,
        const unicode& initial_text="",
        const UIConfig& theme=UIConfig(),
        WidgetStylePtr shared_style=WidgetStylePtr()
    ):
        WidgetParams(theme, shared_style),
        mode(mode), initial_text(initial_text) {}
};

/* A keyboard is combined of a TextInput and a KeyboardPanel */
class Keyboard:
    public Widget {

    DEFINE_SIGNAL(KeyboardKeyPressedSignal, signal_key_pressed);
    DEFINE_SIGNAL(KeyboardDoneSignal, signal_done);
    DEFINE_SIGNAL(KeyboardCancelledSignal, signal_cancelled);
public:
    struct Meta {
        typedef ui::KeyboardParams params_type;
        const static StageNodeType node_type = STAGE_NODE_TYPE_WIDGET_KEYBOARD;
    };

    using Widget::init; // Pull in init to satisfy TwoPhaseConstructed<Keyboard>
    using Widget::clean_up;

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
    bool on_create(void *params) override;
    void on_transformation_change_attempted() override;

    UIDim calculate_content_dimensions(Px text_width, Px text_height) override;

    KeyboardPanel* panel_;
    TextEntry* entry_;
    Frame* info_row_;

    Frame* main_frame_ = nullptr;

    std::shared_ptr<EventListener> keyboard_listener_;

    bool pre_set_text(const unicode& text) override;

    const unicode& calc_text() const;
};

}


}
