#include <Rocket/Core/Element.h>
#include <Rocket/Core/ElementText.h>
#include <Rocket/Core/Context.h>
#include <Rocket/Core/ElementDocument.h>

#include "ui_stage.h"
#include "window_base.h"
#include "loader.h"
#include "ui/ui_private.h"

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

ui::ElementList UIStage::append(const unicode &tag) {
    return interface_->append(tag);
}

ui::ElementList UIStage::$(const unicode &selector) {
    return interface_->_(selector);
}

void UIStage::set_styles(const std::string& styles) {
    return interface_->set_styles(styles);
}

void UIStage::load_rml(const unicode& path) {
    window_.loader_for(path)->into(interface_);
}

void UIStage::load_rml_from_string(const unicode& data) {
    interface_->impl()->document_ = interface_->impl()->context_->LoadDocumentFromMemory(data.encode().c_str());
    if(!interface_->impl()->document_) {
        throw ValueError("Unable to load RML file from data");
    } else {
        interface_->impl()->document_->Show();
    }
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

void UIStage::__handle_mouse_move(int x, int y) {
    interface_->impl()->context_->ProcessMouseMove(x, y, 0); //FIXME pass down modifiers from the Window event
}

void UIStage::__handle_mouse_down(int button) {
    //FIXME: Again, pass down modifiers
    interface_->impl()->context_->ProcessMouseButtonDown(button - 1, 0); //Buttons are zero-based in Rocket land
}

void UIStage::__handle_mouse_up(int button) {
    //FIXME: Again, pass down modifiers
    interface_->impl()->context_->ProcessMouseButtonUp(button - 1, 0);
}

}
