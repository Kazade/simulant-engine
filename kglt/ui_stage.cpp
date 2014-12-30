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

    update_conn_ = window_.signal_step().connect(
        std::bind(&UIStage::__update, this, std::placeholders::_1)
    );
}

UIStage::~UIStage() {
    try {
        update_conn_.disconnect();
    } catch(...) {}
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

void UIStage::register_font_globally(const unicode& ttf_file) {
    this->interface_->load_font(ttf_file);
}

void UIStage::load_rml_from_string(const unicode& data) {
    interface_->impl()->document_->SetInnerRML(data.encode().c_str());
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
    if(!is_being_rendered()) return;

    interface_->impl()->context_->ProcessMouseMove(x, y, 0); //FIXME pass down modifiers from the Window event
}

void UIStage::__handle_mouse_down(int button) {
    if(!is_being_rendered()) return;

    //FIXME: Again, pass down modifiers
    interface_->impl()->context_->ProcessMouseButtonDown(button - 1, 0); //Buttons are zero-based in Rocket land

    mouse_buttons_down_.insert(button);
}

void UIStage::__handle_mouse_up(int button, bool check_rendered) {
    if(check_rendered && !is_being_rendered()) return;

    /* We have this check because we forcibly call mouse up when we stop rendering
     * and without it we might end up triggering a click twice!
     */
    if(mouse_buttons_down_.find(button) != mouse_buttons_down_.end()) {
        //FIXME: Again, pass down modifiers
        interface_->impl()->context_->ProcessMouseButtonUp(button - 1, 0);
        mouse_buttons_down_.erase(button);
    }
}

void UIStage::__handle_touch_up(int finger_id, int x, int y, bool check_rendered) {
    if(check_rendered && !is_being_rendered()) return;

    /* We have this check because we forcibly call touch up when we stop rendering
     * and without it we might end up triggering a click twice!
     */
    if(fingers_down_.find(finger_id) != fingers_down_.end()) {
        interface_->impl()->context_->ProcessTouchUp(finger_id, x, y, 0);
        fingers_down_.erase(finger_id);
    }
}

void UIStage::__handle_touch_motion(int finger_id, int x, int y) {
    if(!is_being_rendered()) return;

    interface_->impl()->context_->ProcessTouchMove(finger_id, x, y, 0);
}

void UIStage::__handle_touch_down(int finger_id, int x, int y) {
    if(!is_being_rendered()) return;

    interface_->impl()->context_->ProcessTouchDown(finger_id, x, y, 0);

    fingers_down_.insert(finger_id);
}

void UIStage::on_render_stopped() {
    // When rendering stops, we need to release all of the touches and buttons (as if the document
    // had been destroyed)

    auto mtmp = mouse_buttons_down_;
    for(auto btn: mtmp) {
        __handle_mouse_up(btn, false);
    }

    auto ftmp = fingers_down_;
    for(auto finger: ftmp) {
        __handle_touch_up(finger, 0, 0, false); //FIXME: Perhaps we should maintain the position?
    }
}

}
