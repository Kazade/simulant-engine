#include "ui_stage.h"
#include "window_base.h"
#include "loader.h"
#include "ui/ui_private.h"

namespace kglt {

UIStage::UIStage(UIStageID id, WindowBase *parent):
    generic::Identifiable<UIStageID>(id),
    Resource(parent->shared_assets.get()),
    window_(parent) {

    resource_manager_ = ResourceManager::create(parent, parent->shared_assets.get());
    interface_ = ui::Interface::create(*parent, this);

    update_conn_ = window_->signal_step().connect(
        std::bind(&UIStage::update, this, std::placeholders::_1)
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
    window_->loader_for("rml_loader", path)->into(interface_);
}

void UIStage::register_font_globally(const unicode& ttf_file) {
    this->interface_->load_font(ttf_file);
}

void UIStage::load_rml_from_string(const unicode& data) {

}

void UIStage::render(CameraPtr camera, Viewport viewport) {
    interface_->render(camera, viewport);
}

void UIStage::update(double dt) {
    interface_->update(dt);
}

void UIStage::__handle_mouse_move(int x, int y) {
    if(!is_being_rendered()) return;

}

void UIStage::__handle_mouse_down(int button) {
    if(!is_being_rendered()) return;

    mouse_buttons_down_.insert(button);
}

void UIStage::__handle_mouse_up(int button, bool check_rendered) {
    if(check_rendered && !is_being_rendered()) return;

    /* We have this check because we forcibly call mouse up when we stop rendering
     * and without it we might end up triggering a click twice!
     */
    if(mouse_buttons_down_.find(button) != mouse_buttons_down_.end()) {
        //FIXME: Again, pass down modifiers
        mouse_buttons_down_.erase(button);
    }
}

void UIStage::__handle_touch_up(int finger_id, int x, int y, bool check_rendered) {
    if(check_rendered && !is_being_rendered()) return;

    /* We have this check because we forcibly call touch up when we stop rendering
     * and without it we might end up triggering a click twice!
     */
    if(fingers_down_.find(finger_id) != fingers_down_.end()) {
        fingers_down_.erase(finger_id);
    }
}

void UIStage::__handle_touch_motion(int finger_id, int x, int y) {
    if(!is_being_rendered()) return;

    /*
     * We don't want to pass down motion events if they weren't preceded by a touch down. This can
     * happen if we stop rendering a UIStage, and we forcibly send touch up messages. This if statement
     * prevents actions being triggered if the UIStage starts rendering when the user has their finger on
     * the screen
     */
    if(fingers_down_.find(finger_id) == fingers_down_.end()) return;

}

void UIStage::__handle_touch_down(int finger_id, int x, int y) {
    if(!is_being_rendered()) return;

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
