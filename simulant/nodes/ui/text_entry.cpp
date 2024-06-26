#include "ui_manager.h"
#include "text_entry.h"
#include "../../font.h"

namespace smlt {
namespace ui {

TextEntry::TextEntry(
    UIManager* owner,
    UIConfig* config,
    Stage* stage,
    std::shared_ptr<WidgetStyle> shared_style
): Widget(owner, config, stage, shared_style) {


}

void TextEntry::insert_character(uint16_t c) {
    auto txt = text();
    auto new_text = txt.substr(0, caret_position_);
    new_text.push_back(c);
    new_text += txt.substr(caret_position_);

    freeze_caret_ = true;
    caret_position_ += 1;
    set_text(new_text);
    freeze_caret_ = false;
}

bool TextEntry::backspace_character() {
    if(caret_position_ == 0) {
        return false;
    }

    auto txt = text();
    auto new_text = txt.substr(0, caret_position_ - 1);
    new_text += txt.substr(caret_position_);

    freeze_caret_ = true;
    caret_position_ -= 1;
    set_text(new_text);
    freeze_caret_ = false;

    return true;
}

bool TextEntry::caret_left() {
    if(caret_position_ == 0) {
        return false;
    }

    caret_position_ -= 1;
    rebuild();
    return true;
}

bool TextEntry::caret_right() {
    if(caret_position_ >= text().length()) {
        return false;
    }

    caret_position_ += 1;
    rebuild();
    return true;
}

Widget::WidgetBounds TextEntry::calculate_foreground_size(const UIDim&) const {
    /* First, find the position of the caret */
    unicode txt = text();
    const auto& fnt = this->font_;
    Px pos = Px(2);
    for(uint16_t i = 0; i < caret_position_; ++i) {
        char16_t c = txt[i];
        pos += Px(fnt->character_advance(c, txt[i + 1]));
    }

    pos -= (this->outer_width() / 2) - border_width() - padding().left;

    Widget::WidgetBounds ret;
    auto width = Px(1);
    auto height = line_height();
    auto bottom = -(height / 2);
    auto top = height - (height / 2);

    ret.min = UICoord(pos, bottom);
    ret.max = UICoord(pos + width, top);

    return ret;
}

bool TextEntry::pre_set_text(const unicode& txt) {
    if(!freeze_caret_) {
        caret_position_ = txt.length();
    }
    return true;
}

}
}

