#include "ui_stage.h"
#include "scene.h"
#include "window_base.h"
#include "loader.h"

namespace kglt {

UIStage::UIStage(Scene *parent, UIStageID id):
    generic::Identifiable<UIStageID>(id),
    Resource(parent),
    scene_(*parent),
    interface_(ui::Interface::create(*parent)){

    scene_.window().signal_step().connect(
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
    scene_.window().loader_for(path)->into(*interface_);
}

void UIStage::__resize(uint32_t width, uint32_t height) {
    interface_->set_dimensions(width, height);
}

void UIStage::__render() {
    interface_->render();
}

void UIStage::__update(double dt) {
    interface_->update(dt);
}

}
