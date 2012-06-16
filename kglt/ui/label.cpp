#include "label.h"

#include "../window_base.h"
#include "../scene.h"
#include "../text.h"

namespace kglt {
namespace ui {

void Label::set_text(const std::string& text) {
    text_object().set_text(text);
    rebuild_meshes();
}

Text& Label::text_object() {
    assert(text_id_);
    return scene().text(text_id_);
}

void Label::_initialize(Scene &scene) {
    text_id_ = scene.new_text(); //Create the text object if we have to
    kglt::Text& text = scene.text(text_id_);
    text.set_parent(this);
    text.move_to(0.0, 0.0, 0.2);

    if(UI* ui = dynamic_cast<UI*>(&parent())) {
        set_font(ui->default_font_id());
    }

    Element::_initialize(scene);
}

const Text& Label::text_object() const {
    assert(text_id_);
    return scene().text(text_id_);
}

double Label::width() {
    return text_object().width_in_pixels();
}

double Label::height() {
    return text_object().font().size();
}

std::string Label::text() const {
    return text_object().text();
}

void Label::set_font(kglt::FontID fid) {
    text_object().apply_font(fid);
}

}
}
