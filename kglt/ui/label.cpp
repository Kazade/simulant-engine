#include "label.h"

#include "../window_base.h"
#include "../scene.h"
#include "../text.h"
#include "../ui.h"

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

    text.move_to(padding_left(), padding_bottom(), 0.2);

    set_font(ui().default_font_id());

    Element::_initialize(scene);

    text.set_parent(&background());
}

const Text& Label::text_object() const {
    assert(text_id_);
    return scene().text(text_id_);
}

double Label::width() {
    return text_object().width_in_pixels();
}

double Label::height() {
    return float(text_object().font().size()) * 1.5;
}

std::string Label::text() const {
    return text_object().text();
}

void Label::set_font(kglt::FontID fid) {
    text_object().apply_font(fid);
}

}
}
