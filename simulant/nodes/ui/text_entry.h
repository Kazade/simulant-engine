#pragma once

#include "widget.h"
#include "../../generic/managed.h"

namespace smlt {
namespace ui {

class TextEntry:
    public Widget,
    public RefCounted<TextEntry> {

public:
    using Widget::init; // Pull in init to satisfy Managed<TextEntry>
    using Widget::clean_up;

    TextEntry(
        UIManager* owner,
        UIConfig* config,
        Stage* stage,
        std::shared_ptr<WidgetStyle> shared_style=std::shared_ptr<WidgetStyle>()
    );

    /* Inserts a character at the caret position */
    void insert_character(uint16_t c);

    /* Deletes the character before the caret, returns
     * false if the caret was at position 0 */
    bool backspace_character();

    /* Moves the caret 1 character left, returns false if
     * the caret was already at position 0 */
    bool caret_left();

    /* Moves the caret 1 character right, returns false if
     * the caret was already at the last character position */
    bool caret_right();

private:
    /* This is the character position of the caret. 0 means it's before the
     * first character */
    uint16_t caret_position_ = 0;

    /* If true, then set_text won't update the caret position */
    bool freeze_caret_ = false;

    /* The caret is rendered using the foreground layer. Usually 1px wide and the
     * same height as the font. The position will depend on the caret_position_
     * and the text */
    virtual WidgetBounds calculate_foreground_size(const UIDim& content_dimensions) const override;

    virtual bool pre_set_text(const unicode&) override;
};

}
}
