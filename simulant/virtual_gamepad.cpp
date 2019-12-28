//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "logging.h"
#include "utils/unicode.h"
#include "virtual_gamepad.h"
#include "window.h"
#include "stage.h"
#include "render_sequence.h"
#include "nodes/ui/ui_manager.h"
#include "nodes/ui/button.h"


namespace smlt {

VirtualGamepad::VirtualGamepad(Window &window, VirtualGamepadConfig config):
    window_(window),
    config_(config) {

}

AABB VirtualGamepad::button_bounds(int button) {
    return buttons_[button]->transformed_aabb();
}

bool VirtualGamepad::init() {
    stage_ = window_.new_stage(); //Create a Stage to hold the controller buttons
    stage_id_ = stage_->id();

    uint32_t button_size = int(float(window_.width() / 10.0));

    if(this->config_ == VIRTUAL_GAMEPAD_CONFIG_TWO_BUTTONS) {
        auto button1 = stage_->ui->new_widget_as_button("L");
        auto button2 = stage_->ui->new_widget_as_button("R");

        button1->set_background_colour(smlt::Colour(0, 0, 0, 0.2));
        button2->set_background_colour(smlt::Colour(0, 0, 0, 0.2));

        button1->set_font(stage_->assets->default_font(DEFAULT_FONT_STYLE_HEADING));
        button2->set_font(stage_->assets->default_font(DEFAULT_FONT_STYLE_HEADING));

        button1->move_to(window_.coordinate_from_normalized(0.10f, 0.10f * window_.aspect_ratio()));
        button2->move_to(window_.coordinate_from_normalized(0.90f, 0.10f * window_.aspect_ratio()));

        button1->resize(button_size, button_size);
        button2->resize(button_size, button_size);

        connections_.push_back(button1->signal_pressed().connect([this]() {
            signal_button_down_(0);
        }));

        connections_.push_back(button2->signal_pressed().connect([this]() {
            signal_button_down_(1);
        }));

        connections_.push_back(button1->signal_released().connect([this]() {
            signal_button_up_(0);
        }));

        connections_.push_back(button2->signal_released().connect([this]() {
            signal_button_up_(1);
        }));

        buttons_.push_back(button1);
        buttons_.push_back(button2);
    } else {
        assert(0 && "Not Implemented");
    }

    camera_ = stage_->new_camera_with_orthographic_projection();

    //Finally add to the render sequence, give a ridiculously high priority
    pipeline_ = window_.render(stage_, camera_).with_priority(smlt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);
    pipeline_->activate();
    pipeline_id_ = pipeline_->id();

    return true;
}

void VirtualGamepad::_prepare_deletion() {
    pipeline_->deactivate();
}

void VirtualGamepad::clean_up() {
    L_DEBUG("Destroying virtual gamepad");

    // make sure we delete the buttons before we delete the gamepad
    // This will fire any release signals
    for(auto& button: buttons_) {
        stage_->ui->destroy_widget(button->id());
    }

    // Remove any signal connections
    connections_.clear();

    if(window_.has_pipeline(pipeline_id_)) {
        pipeline_ = window_.destroy_pipeline(pipeline_id_);
    }

    if(window_.has_stage(stage_id_)) {
        stage_ = window_.destroy_stage(stage_id_);
    }
}

void VirtualGamepad::flip() {

}

}
