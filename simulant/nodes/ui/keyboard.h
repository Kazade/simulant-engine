#pragma once

#include <map>
#include "widget.h"
#include "../../keycodes.h"

namespace smlt {
namespace ui {

/** An onscreen keyboard.
 */

class Frame;

enum KeyboardLayout {
    KEYBOARD_LAYOUT_ALPHABETICAL,
    KEYBOARD_LAYOUT_NUMERICAL,
};

typedef sig::signal<void (char)> KeyboardActivatedSignal;
typedef sig::signal<void ()> KeyboardDoneSignal;

class Keyboard:
    public Widget {

    DEFINE_SIGNAL(KeyboardActivatedSignal, signal_activated);
    DEFINE_SIGNAL(KeyboardDoneSignal, signal_done);
public:
    using Widget::init; // Pull in init to satisfy Managed<Image>
    using Widget::clean_up;

    Keyboard(UIManager* owner, UIConfig* config, KeyboardLayout layout);
    ~Keyboard();

    void move_up();
    void move_down();
    void move_right();
    void move_left();
    void activate();

    void set_target(smlt::ui::Widget* widget);

    /** This only enables the keys that appear in the passed list, passing
     *  an empty string will remove any limitation */
    void limit_chars_to(const unicode& char_list);

    using Widget::set_font;
private:
    void move_row(int dir);

    std::shared_ptr<WidgetStyle> default_style_;
    std::shared_ptr<WidgetStyle> highlighted_style_;
    std::shared_ptr<WidgetStyle> disabled_style_;

    KeyboardLayout layout_ = KEYBOARD_LAYOUT_ALPHABETICAL;
    unicode limited_chars_;
    // utf-16
    std::map<uint16_t, Button*> buttons_;
    void set_enabled(Button* btn, bool value);


    Frame* main_frame_ = nullptr;
    Frame* rows_[5] = {0, 0, 0, 0, 0};
    smlt::ui::Widget* focused_ = nullptr;
    smlt::ui::Widget* target_ = nullptr;
    sig::Connection target_destroyed_;

    void clear();

    void generate_alphabetical_layout();
    void generate_numerical_layout();

    void focus(smlt::ui::Widget* widget);

    void unfocus(smlt::ui::Widget* widget);

    virtual UIDim calculate_content_dimensions(Px text_width, Px text_height) override;

    void on_transformation_change_attempted() override;
    void set_font(FontPtr font) override;

    smlt::ui::Button* new_button(const unicode& label);
};

}
}
