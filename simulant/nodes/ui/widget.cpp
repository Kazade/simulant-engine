#include "widget.h"
#include "ui_manager.h"
#include "../actor.h"
#include "../../stage.h"
#include "../../material.h"

namespace smlt {
namespace ui {

Widget::Widget(WidgetID id, UIManager *owner, UIConfig *defaults):
    StageNode(owner->stage()),
    generic::Identifiable<WidgetID>(id),
    owner_(owner) {

}

bool Widget::init() {
    actor_ = stage->new_actor();
    actor_.fetch()->set_parent(this);

    on_size_changed(); // Build the mesh and attach to the actor

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

void Widget::on_size_changed() {
    auto mesh_id = construct_widget(width_, height_);
    mesh_ = mesh_id.fetch(); // Store a handle to the mesh

    actor_.fetch()->set_mesh(mesh_id);
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

void Widget::set_background_colour(const Colour& colour) {
    background_colour_ = colour;
}

void Widget::set_foreground_colour(const Colour& colour) {
    foreground_colour_ = colour;
}

MeshID Widget::construct_widget(float requested_width, float requested_height) {
    VertexSpecification spec;
    spec.position_attribute = VERTEX_ATTRIBUTE_3F;
    spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    spec.diffuse_attribute = VERTEX_ATTRIBUTE_4F;

    MaterialID material = stage->assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);

    auto content_dim = calc_content_dimensions();

    float width = requested_width;
    float height = requested_height;

    switch(resize_mode_) {
        case RESIZE_MODE_FIT_CONTENT: {
            width = content_dim.width + padding_.left + padding_.right;
            height = content_dim.height + padding_.top + padding_.bottom;
        }; break;
        case RESIZE_MODE_AT_LEAST_CONTENT: {
            width = std::max(width, content_dim.width + padding_.left + padding_.right);
            height = std::max(height, content_dim.height + padding_.top + padding_.bottom);
        } break;
        default:
            break; // FIXED
    }

    auto mesh = stage->assets->new_mesh(spec).fetch();
    mesh->new_submesh_as_rectangle("border", material, width + (border_width_ * 2), height + (border_width_ * 2));
    mesh->submesh("border")->set_diffuse(border_colour_);

    mesh->new_submesh_as_rectangle("content", material, width, height);
    mesh->submesh("content")->set_diffuse(background_colour_);

    // Make sure the mesh doesn't get cleaned up until next access
    stage->assets->MeshManager::mark_as_uncollected(mesh->id());

    return mesh->id();
}

void Widget::set_resize_mode(ResizeMode resize_mode) {
    resize_mode_ = resize_mode;
}

void Widget::set_padding(float left, float right, float bottom, float top) {
    padding_.left = left;
    padding_.right = right;
    padding_.bottom = bottom;
    padding_.top = top;
}

UIDim Widget::calc_content_dimensions() {
    UIDim ret;
    ret.width = 80; //TEMPORARY UNTIL FONT RENDERING
    ret.height = 16;
    return ret;
}

}
}
