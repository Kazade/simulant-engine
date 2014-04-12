#include "ui_stage.h"
#include "scene.h"
#include "window_base.h"
#include "loader.h"

namespace kglt {

UIStage::UIStage(WindowBase *parent, UIStageID id):
    generic::Identifiable<UIStageID>(id),
    Resource(parent),
    window_(*parent),
    interface_(ui::Interface::create(*parent)){

    window_.signal_step().connect(
        std::bind(&UIStage::__update, this, std::placeholders::_1)
    );
}

ui::Element UIStage::append(const std::string& tag) {
    return interface_->append(tag);
}

ui::ElementList UIStage::$(const std::string& selector) {
    return interface_->_(selector);
}

void UIStage::set_styles(const std::string& styles) {
    return interface_->set_styles(styles);
}

void UIStage::load_rml(const unicode& path) {
    window_.loader_for(path)->into(interface_);
}

void UIStage::__resize(uint32_t width, uint32_t height) {
    interface_->set_dimensions(width, height);
}

void UIStage::__render(const Mat4 &projection_matrix) {
    interface_->render(projection_matrix);
}

void UIStage::__update(double dt) {
    interface_->update(dt);
}

}
