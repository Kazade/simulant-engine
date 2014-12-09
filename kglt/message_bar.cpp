#include "window_base.h"
#include "message_bar.h"
#include "render_sequence.h"
#include "ui_stage.h"
#include "camera.h"

namespace kglt {

MessageBar::MessageBar(WindowBase& parent):
    window_(parent) {

    update_conn_ = parent.signal_step().connect(std::bind(&MessageBar::update, this, std::placeholders::_1));
}

MessageBar::~MessageBar() {
    try {
        update_conn_.disconnect();
    } catch(...) {}
}

bool MessageBar::init() {
    create_stage_and_element();
    return true;
}

void MessageBar::display_message(Message next_message) {
    auto ui = window_.ui_stage(stage_);

    ui->$("#message-bar").text(next_message.text);
    switch(next_message.type) {
        case MessageType::ALERT:
            ui->$("#message-bar").attr("background-color", "#ff000088");
        break;
        case MessageType::WARN:
            ui->$("#message-bar").attr("background-color", "#ffff0088");
        break;
        case MessageType::INFORM:
            ui->$("#message-bar").attr("background-color", "#0000ff88");
        break;
        default:
            ui->$("#message-bar").attr("background-color", "#ffffff88");
    }
}

void MessageBar::update(float dt) {
    auto ui = window_.ui_stage(stage_);

    if(ui->$("#message-bar").empty()) {
        return;
    }

    if(ui->$("#message-bar")[0].is_visible()) {
        time_message_visible_ += dt;
        if(time_message_visible_ > 3.0) {
            if(!message_queue_.empty()) {
                Message next_message = message_queue_.front();
                display_message(next_message);
                message_queue_.pop();
            } else {
                //No more messages, let's hide the bar
                ui->$("#message-bar").hide();
            }
            time_message_visible_ = 0.0f;
        }
    } else {
        //Not visible
        if(!message_queue_.empty()) {
            //We have messages, let's display them
            Message next_message = message_queue_.front();
            display_message(next_message);
            message_queue_.pop();

            ui->$("#message-bar").show();
            time_message_visible_ = 0.0f;
        }
    }
}

void MessageBar::create_stage_and_element() {
    if(stage_) {
        return;
    }

    stage_ = window_.new_ui_stage();
    camera_ = window_.new_camera();

    window_.camera(camera_)->set_orthographic_projection(0, window_.width(), window_.height(), 0, -1, 1);

    {
        auto ui = window_.ui_stage(stage_);
        ui->append("<div>").id("message-bar");
        auto $element = ui->$("#message-bar");
        $element.css("position", "absolute");
        $element.css("display", "block");
        $element.hide();
    }

    window_.render_sequence()->new_pipeline(stage_, camera_, ViewportID(), TextureID(), 1000);
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
