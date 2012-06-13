#include "label.h"

#include "../window_base.h"
#include "../scene.h"
#include "../text.h"

namespace kglt {
namespace ui {

void Label::set_text(const std::string& text) {
    text_object().set_text(text);
}

Text& Label::text_object() {
    return scene().text(text_id_);
}

double Label::width() {
    return text_object().width_in_pixels();
}

double Label::height() {
    return text_object().font().size();
}

void Label::on_parent_set(Object *old_parent) {
    Element::on_parent_set(old_parent);

    if(!text_id_) {
        text_id_ = scene().new_text(); //Create the text object if we have to
    }
}

}
}
