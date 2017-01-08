//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "window_base.h"
#include "message_bar.h"
#include "render_sequence.h"
#include "overlay.h"
#include "camera.h"

namespace smlt {

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
    auto ui = window_.overlay(stage_);

    ui->find("#message-bar").set_text(next_message.text);
    switch(next_message.type) {
        case MessageType::ALERT:
            ui->find("#message-bar").set_attr("background-color", "#ff000088");
        break;
        case MessageType::WARN:
            ui->find("#message-bar").set_attr("background-color", "#ffff0088");
        break;
        case MessageType::INFORM:
            ui->find("#message-bar").set_attr("background-color", "#0000ff88");
        break;
        default:
            ui->find("#message-bar").set_attr("background-color", "#ffffff88");
    }
}

void MessageBar::update(float dt) {
    auto ui = window_.overlay(stage_);

    if(ui->find("#message-bar").empty()) {
        return;
    }

    if(ui->find("#message-bar")[0].is_visible()) {
        time_message_visible_ += dt;
        if(time_message_visible_ > 3.0) {
            if(!message_queue_.empty()) {
                Message next_message = message_queue_.front();
                display_message(next_message);
                message_queue_.pop();
            } else {
                //No more messages, let's hide the bar
                ui->find("#message-bar").hide();
                window_.disable_pipeline(pipeline_id_);
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

            window_.enable_pipeline(pipeline_id_);
            ui->find("#message-bar").show();

            time_message_visible_ = 0.0f;
        }
    }
}

void MessageBar::create_stage_and_element() {
    if(stage_) {
        return;
    }

    stage_ = window_.new_overlay();
    camera_ = window_.new_camera();

    window_.camera(camera_)->set_orthographic_projection(0, window_.width(), window_.height(), 0, -1, 1);

    {
        auto ui = window_.overlay(stage_);
        ui->append_row().append_label("").set_id("message-bar");
        auto $element = ui->find("#message-bar");
        $element.hide();
    }

    pipeline_id_ = window_.render(stage_, camera_).with_priority(smlt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);
    window_.disable_pipeline(pipeline_id_);
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
