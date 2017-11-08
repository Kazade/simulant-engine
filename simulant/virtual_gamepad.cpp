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

#include "deps/kazlog/kazlog.h"
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

    uint32_t button_size = int(float(window_.width() / 10.0));

    if(this->config_ == VIRTUAL_GAMEPAD_CONFIG_TWO_BUTTONS) {
        auto button1 = stage_->ui->new_widget_as_button("L").fetch_as<ui::Button>();
        auto button2 = stage_->ui->new_widget_as_button("R").fetch_as<ui::Button>();

        button1->set_background_colour(smlt::Colour(0, 0, 0, 0.2));
        button2->set_background_colour(smlt::Colour(0, 0, 0, 0.2));

        button1->set_font(stage_->assets->default_font_id(DEFAULT_FONT_STYLE_HEADING));
        button2->set_font(stage_->assets->default_font_id(DEFAULT_FONT_STYLE_HEADING));

        button1->move_to(window_.coordinate_from_normalized(0.10, 0.10 * window_.aspect_ratio()));
        button2->move_to(window_.coordinate_from_normalized(0.90, 0.10 * window_.aspect_ratio()));

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

    camera_id_ = stage_->new_camera_with_orthographic_projection();

    //Finally add to the render sequence, give a ridiculously high priority
    pipeline_id_ = window_.render(stage_->id(), camera_id_).with_priority(smlt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);

    return true;
}

void VirtualGamepad::_prepare_deletion() {
    window_.disable_pipeline(pipeline_id_);

    // make sure we delete the buttons before we delete the gamepad
    for(auto& button: buttons_) {
        stage_->ui->delete_widget(button->id());
    }
}

void VirtualGamepad::cleanup() {
    L_DEBUG("Destroying virtual gamepad");

    window_.delete_pipeline(pipeline_id_);
    window_.delete_stage(stage_->id());
}

void VirtualGamepad::flip() {

}

}
