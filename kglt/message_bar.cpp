#include "window_base.h"
#include "scene.h"
#include "message_bar.h"
#include "render_sequence.h"

namespace kglt {

MessageBar::MessageBar(WindowBase& parent):
    window_(parent) {
}

bool MessageBar::init() {
    create_stage_and_element();
    return true;
}

void MessageBar::update(float dt) {
    if(ui->$("#message-bar").empty()) {
        return;
    }

    auto ui = window_.scene().ui_stage(stage_);

}

void MessageBar::create_stage_and_element() {
    if(stage_) {
        return;
    }

    stage_ = window_.scene().new_ui_stage();
    camera_ = window_.scene().new_camera();

    {
        auto cam = window_.scene().camera_ref(camera_).lock();
        cam->set_orthographic_projection(0, window_.width(), window_.height(), 0, -1, 1);
    }

    {
        auto ui = window_.scene().ui_stage(stage_);
        ui->append("<div>").id("message-bar");
        auto $element = ui->$("#message-bar");
        $element.css("position", "absolute");
    }

    window_.scene().render_sequence().new_pipeline(stage_, camera_, ViewportID(), TextureID(), 1000);
}

void MessageBar::notify_left(const unicode& message) {
    message_queue_.push(Message{MessageType::NOTIFY_LEFT, message});
}

void MessageBar::notify_right(const unicode& message) {
    message_queue_.push(Message{MessageType::NOTIFY_RIGHT, message});
}

void MessageBar::inform(const unicode& message) {
    message_queue_.push(Message{MessageType::INFORM, message});
}

void MessageBar::warn(const unicode& warning) {
    message_queue_.push(Message{MessageType::WARN, warning});
}

void MessageBar::alert(const unicode& alert) {
    message_queue_.push(Message{MessageType::ALERT, alert});
}

}
