#include "label.h"

namespace kglt {
namespace ui {

void Label::set_text(const std::string& text) {
    text_ = text;
}

void Label::set_position(float x, float y) {
    double parent_width = 0.0;
    double parent_height = 0.0;
    double parent_left = 0.0;
    double parent_top = 0.0;

    ui::Element* parent_element = nullptr;
    if(has_parent()) {
        parent_element = dynamic_cast<ui::Element*>(&parent());
    }

    if(parent_element) {
        parent_width = parent_ui->width();
        parent_height = parent_ui->height();
        parent_left = parent->position().x;
        parent_top = parent->position().y;
    } else {
        parent_width = scene().window().width();
        parent_height = scene().window().height();
    }

    position().x = parent_left + (x * parent_width);
    position().y = parent_top + (y * parent_height);
}

}
}
