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
#include "window_base.h"
#include "stage.h"
#include "render_sequence.h"

namespace smlt {

VirtualGamepad::VirtualGamepad(WindowBase &window, VirtualDPadDirections directions, int button_count):
    window_(window),
    directions_(directions),
    button_count_(button_count) {

}

Dimensions VirtualGamepad::button_dimensions(int button) {
    if(button >= button_count_) {
        throw std::out_of_range(_u("Invalid button: {0}").format(button).encode());
    }

    button++; //Buttons are 1-indexed in the UI element classes
/*
    auto document = window_.overlay(overlay_);

    auto klass = _u(".button_{0}").format(humanize(button)).encode();

    ui::ElementList list = document->find(klass);
    if(list.empty()) {
        throw std::logic_error(_u("Unable to find button: {0}").format(button).encode());
    }

    ui::Element element = list[0];
*/
    Dimensions dim;

    //FIXME: !!!!!!!!!!!

    /*dim.left = element.left();
    dim.top = element.top();
    dim.width = element.width();
    dim.height = element.height();*/

    return dim;
}

bool VirtualGamepad::init() {
    L_DEBUG(_F("Initializing virtual gamepad with {0} buttons").format(button_count_));

    stage_ = window_.new_stage(); //Create a Stage to hold the controller buttons

    uint32_t button_size = int(float(window_.width() / 10.0));
    uint32_t dpad_margin = int(float(window_.width() * (5.0 / 640.0)));
    uint32_t area_width = int(float(window_.width() * (200.0 / 640.0)));
    uint32_t outside_padding = int(float(window_.width() * (10.0 / 640.0)));
    uint32_t font_size = window_.height() / 50;

    auto stage = stage_.fetch();

/*
    stage->load_rml_from_string(layout.format(button_size, dpad_margin, area_width, outside_padding, font_size));

    if(this->directions_ == VIRTUAL_DPAD_DIRECTIONS_TWO) {
        stage->find(".dpad_two").add_css("display", "inline-block");


        for(auto evt: { ui::EVENT_TYPE_TOUCH_DOWN, ui::EVENT_TYPE_TOUCH_OVER}) {
            stage->find(".dpad_left").set_event_callback(evt, [=](ui::Event evt) -> bool {
                dpad_button_touches_["left"].insert(evt.touch.finger_id);
                if(dpad_button_touches_["left"].size() == 1) {
                    signal_hat_changed_(HAT_POSITION_LEFT);
                }
                return true;
            });

            stage->find(".dpad_right").set_event_callback(evt, [=](ui::Event evt) -> bool {
                dpad_button_touches_["right"].insert(evt.touch.finger_id);
                if(dpad_button_touches_["right"].size() == 1) {
                    signal_hat_changed_(HAT_POSITION_RIGHT);
                }
                return true;
            });
        }

        for(auto evt: { ui::EVENT_TYPE_TOUCH_UP, ui::EVENT_TYPE_TOUCH_OUT}) {
            stage->find(".dpad_left").set_event_callback(evt, [=](ui::Event evt) -> bool {
                dpad_button_touches_["left"].erase(evt.touch.finger_id);
                if(dpad_button_touches_["left"].size() == 0) {
                    signal_hat_changed_(HAT_POSITION_CENTERED);
                }
                return true;
            });

            stage->find(".dpad_right").set_event_callback(evt, [=](ui::Event evt) -> bool {
                dpad_button_touches_["right"].erase(evt.touch.finger_id);
                if(dpad_button_touches_["right"].size() == 0) {
                    signal_hat_changed_(HAT_POSITION_CENTERED);
                }
                return true;
            });
        }
    }

    //Make the buttons visible that need to be
    for(int i = 1; i < button_count_ + 1; ++i) {
        int idx = i - 1;

        auto klass = _u(".button_{0}").format(humanize(i)).encode();
        stage->find(klass).add_css("display", "inline-block");

        for(auto evt: { ui::EVENT_TYPE_TOUCH_DOWN, ui::EVENT_TYPE_TOUCH_OVER}) {
            //Forward the events from the UI on to the signals
            stage->find(klass).set_event_callback(evt, [=](ui::Event evt) -> bool {
                this->button_touches_[idx].insert(evt.touch.finger_id);
                if(this->button_touches_[idx].size() == 1) {
                    signal_button_down_(idx);
                }
                return true;
            });
        }

        for(auto evt: { ui::EVENT_TYPE_TOUCH_UP, ui::EVENT_TYPE_TOUCH_OUT}) {
            stage->find(klass).set_event_callback(evt, [=](ui::Event evt) -> bool {
                this->button_touches_[idx].erase(evt.touch.finger_id);
                if(this->button_touches_[idx].size() == 0) {
                    signal_button_up_(i - 1);
                }
                return true;
            });
        }
    }

    camera_id_ = window_.new_camera_with_orthographic_projection();

    //Finally add to the render sequence, give a ridiculously high priority
    pipeline_id_ = window_.render(overlay_, camera_id_).with_priority(smlt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);
*/
    return true;
}

void VirtualGamepad::_disable_rendering() {
    window_.disable_pipeline(pipeline_id_);
}

void VirtualGamepad::cleanup() {
    L_DEBUG("Destroying virtual gamepad");

    window_.delete_pipeline(pipeline_id_);
    window_.delete_camera(camera_id_);
    window_.delete_stage(stage_);
}

void VirtualGamepad::flip() {

}

}
