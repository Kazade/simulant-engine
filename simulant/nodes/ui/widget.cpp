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

MeshID Widget::construct_widget(float width, float height) {
    VertexSpecification spec;
    spec.position_attribute = VERTEX_ATTRIBUTE_3F;
    spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    spec.diffuse_attribute = VERTEX_ATTRIBUTE_4F;

    MaterialID material = stage->assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);

    auto mesh = stage->assets->new_mesh(spec).fetch();
    mesh->new_submesh_as_rectangle("border", material, width, height);
    mesh->new_submesh_as_rectangle("content", material, width, height);

    // Make sure the mesh doesn't get cleaned up until next access
    stage->assets->MeshManager::mark_as_uncollected(mesh->id());

    return mesh->id();
}

UIDim Widget::calc_content_dimensions() {
    return UIDim();
}

}
}
