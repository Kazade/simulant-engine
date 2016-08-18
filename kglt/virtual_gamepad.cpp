#include <kazbase/logging.h>
#include <kazbase/unicode.h>

#include "virtual_gamepad.h"
#include "window_base.h"
#include "overlay.h"
#include "ui/interface.h"
#include "render_sequence.h"

namespace kglt {

unicode layout = R"(
<rml>
    <head>
        <style>
            body {
                height: 100%;
                font-family: Ubuntu;
                font-weight: bold;
                font-size: {4}px;
                color: #ffffff88;
            }
            div {
                display: block;
            }

            .controls {
                position: absolute;
                bottom: 12%;
                padding-left: 2%;
                padding-right: 2%;
            }

            .buttons {
                width: {2}px;
                float: right;
                margin-right: {3}px;
                text-align: right;
                margin-bottom: {3}px;
            }

            .dpad {
                width: {2}px;
                float: left;
                margin-left: {3}px;
                margin-bottom: {3}px;
            }

            .dpad, .button {
                display: none;
            }

            .dpad_left {
                width: {0}px;
                height: {0}px;
                background-image: "kglt/materials/textures/button.png";
                background-decorator: image;
                margin-left: {1}px;
                margin-right: {1}px;
                text-align: center;
                line-height: {0}px;
                font-size: 2em;
                float: left;
            }

            .dpad_right {
                width: {0}px;
                height: {0}px;
                background-image: "kglt/materials/textures/button.png";
                background-decorator: image;
                margin-left: {1}px;
                margin-right: {1}px;
                text-align: center;
                line-height: {0}px;
                font-size: 2em;
                float: left;
            }

            .button {
                width: {0}px;
                height: {0}px;
                text-align: center;
                line-height: {0}px;
                vertical-align: middle;
                background-decorator: image;
                background-image: "kglt/materials/textures/button.png";
                margin-left: {1}px;
                margin-right: {1}px;
                font-size: 2em;
            }

            .button_text { display: none; }

        </style>
    </head>
    <body>
        <div class="controls">
            <div class="dpad dpad_two">
                <div class="dpad_left">L</div>
                <div class="dpad_right">R</div>
            </div>
            <div class="dpad dpad_four">

            </div>
            <div class="dpad dpad_eight">

            </div>

            <div class="buttons">
                <div class="button button_one">1</div>
                <div class="button button_two">2</div>
                <div class="button button_three">3</div>
            </div>
        </div>
    </body>
</rml>
)";


VirtualGamepad::VirtualGamepad(WindowBase &window, VirtualDPadDirections directions, int button_count):
    window_(window),
    directions_(directions),
    button_count_(button_count) {

}

Dimensions VirtualGamepad::button_dimensions(int button) {
    if(button >= button_count_) {
        throw ValueError(_u("Invalid button: {0}").format(button));
    }

    button++; //Buttons are 1-indexed in the UI element classes

    auto document = window_.overlay(overlay_);

    unicode klass = _u(".button_{0}").format(humanize(button));

    ui::ElementList list = document->$(klass);
    if(list.empty()) {
        throw ValueError(_u("Unable to find button: {0}").format(button));
    }

    ui::Element element = list[0];

    Dimensions dim;

    dim.left = element.left();
    dim.top = element.top();
    dim.width = element.width();
    dim.height = element.height();

    return dim;
}

bool VirtualGamepad::init() {
    L_DEBUG(_u("Initializing virtual gamepad with {0} buttons").format(button_count_));

    overlay_ = window_.new_overlay(); //Create a UI stage to hold the controller buttons

    uint32_t button_size = int(float(window_.width() / 10.0));
    uint32_t dpad_margin = int(float(window_.width() * (5.0 / 640.0)));
    uint32_t area_width = int(float(window_.width() * (200.0 / 640.0)));
    uint32_t outside_padding = int(float(window_.width() * (10.0 / 640.0)));
    uint32_t font_size = window_.height() / 50;
    auto stage = window_.overlay(overlay_);
    stage->load_rml_from_string(layout.format(button_size, dpad_margin, area_width, outside_padding, font_size));

    if(this->directions_ == VIRTUAL_DPAD_DIRECTIONS_TWO) {
        stage->$(".dpad_two").css("display", "inline-block");


        for(auto evt: { "touchdown", "touchover"}) {
            stage->$(".dpad_left").set_event_callback(evt, [=](ui::Event evt) -> bool {
                dpad_button_touches_["left"].insert(evt.touch.finger_id);
                if(dpad_button_touches_["left"].size() == 1) {
                    signal_hat_changed_(HAT_POSITION_LEFT);
                }
                return true;
            });

            stage->$(".dpad_right").set_event_callback(evt, [=](ui::Event evt) -> bool {
                dpad_button_touches_["right"].insert(evt.touch.finger_id);
                if(dpad_button_touches_["right"].size() == 1) {
                    signal_hat_changed_(HAT_POSITION_RIGHT);
                }
                return true;
            });
        }

        for(auto evt: { "touchup", "touchout"}) {
            stage->$(".dpad_left").set_event_callback(evt, [=](ui::Event evt) -> bool {
                dpad_button_touches_["left"].erase(evt.touch.finger_id);
                if(dpad_button_touches_["left"].size() == 0) {
                    signal_hat_changed_(HAT_POSITION_CENTERED);
                }
                return true;
            });

            stage->$(".dpad_right").set_event_callback(evt, [=](ui::Event evt) -> bool {
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

        unicode klass = _u(".button_{0}").format(humanize(i));
        stage->$(klass).css("display", "inline-block");

        for(auto evt: { "touchdown", "touchover"}) {
            //Forward the events from the UI on to the signals
            stage->$(klass).set_event_callback(evt, [=](ui::Event evt) -> bool {
                this->button_touches_[idx].insert(evt.touch.finger_id);
                if(this->button_touches_[idx].size() == 1) {
                    signal_button_down_(idx);
                }
                return true;
            });
        }

        for(auto evt: { "touchup", "touchout"}) {
            stage->$(klass).set_event_callback(evt, [=](ui::Event evt) -> bool {
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
    pipeline_id_ = window_.render(overlay_, camera_id_).with_priority(kglt::RENDER_PRIORITY_ABSOLUTE_FOREGROUND);

    return true;
}

void VirtualGamepad::_disable_rendering() {
    window_.disable_pipeline(pipeline_id_);
}

void VirtualGamepad::cleanup() {
    L_DEBUG("Destroying virtual gamepad");

    window_.delete_pipeline(pipeline_id_);
    window_.delete_camera(camera_id_);
    window_.delete_overlay(overlay_);
}

void VirtualGamepad::flip() {

}

}
