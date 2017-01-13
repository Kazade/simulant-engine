#include "widget.h"
#include "ui_manager.h"
#include "../actor.h"

namespace smlt {
namespace ui {

Widget::Widget(WidgetID id, UIManager *owner, UIConfig *defaults):
    StageNode(owner->stage()),
    generic::Identifiable<WidgetID>(id),
    owner_(owner) {

}

bool Widget::init() {
    actor_ = construct_widget();
    actor_.fetch()->set_parent(this);
    return true;
}

void Widget::resize(float width, float height) {
    width_ = width;
    height_ = height;
    on_size_changed();
}

void Widget::set_width(float width) {
    width_ = width;
    on_size_changed();
}

void Widget::set_height(float height) {
    height_ = height;
    on_size_changed();
}

void Widget::set_property(const std::string &name, float value) {
    properties_[name] = value;
}

void Widget::ask_owner_for_destruction() {
    owner_->delete_widget(id());
}

const AABB Widget::aabb() const {
    return actor_.fetch()->aabb();
}

ActorID Widget::construct_widget() {
    return ActorID();
}

UIDim Widget::calc_content_dimensions() {
    return UIDim();
}

}
}
