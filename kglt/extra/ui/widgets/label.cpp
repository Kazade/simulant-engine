
#include "label.h"
#include "../interface.h"
#include "../../../scene.h"

namespace kglt {
namespace extra {
namespace ui {

Label::Label(Interface* interface, LabelID id):
    Widget(*interface),
    generic::Identifiable<LabelID>(id) {

}

void Label::set_text(const std::string& text) {
    uint16_t height = height_in_pixels() - interface().default_size() - this->padding().top;
    text_ = Text::create(interface()._font(interface().default_font(), interface().default_size()), Vec2(this->padding().left, height), text);
    text_->set_colour(foreground_colour());

    interface().subscene().entity(text_->entity_id()).set_parent(&interface().subscene().entity(entity_id()));
}

void Label::on_resize() {
    if(!text_) return;

    text_->set_position(Vec2(this->padding().left, height_in_pixels() - interface().default_size() - this->padding().top));
}

void Label::on_foreground_colour_changed() {
    if(!text_) return;

    text_->set_colour(foreground_colour());
}

}
}
}
