#include <kazbase/logging.h>
#include <kazbase/unicode.h>

#include "virtual_gamepad.h"
#include "window_base.h"
#include "ui_stage.h"
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
                font-size: 2em;
            }
            div {
                display: block;
            }

            .controls {
                position: absolute;
                bottom: 0px;
                left: 0px;
                right: 0px;
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
                background-image: "kglt/materials/left.png";
                background-decorator: image;
                margin-left: {1}px;
                margin-right: {1}px;
                float: left;
            }

            .dpad_right {
                width: {0}px;
                height: {0}px;
                background-image: "kglt/materials/right.png";
                background-decorator: image;
                margin-left: {1}px;
                margin-right: {1}px;
                float: left;
            }

            .button {
                width: {0}px;
                height: {0}px;
                text-align: center;
                vertical-align: middle;
                background-decorator: image;
                background-image: "kglt/materials/button.png";
                margin-left: {1}px;
                margin-right: {1}px;
            }

            .button_text { display: none; }

        </style>
    </head>
    <body>
        <div class="controls">
            <div class="dpad dpad_two">
                <div class="dpad_left"></div>
                <div class="dpad_right"></div>
            </div>
            <div class="dpad dpad_four">

            </div>
            <div class="dpad dpad_eight">

            </div>

            <div class="buttons">
                <div class="button button_one">
                    <span class="button_text">1</span>
                </div>
                <div class="button button_two">
                    <span class="button_text">2</span>
                </div>
                <div class="button button_three">
                    <span class="button_text">3</span>
                </div>
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

bool VirtualGamepad::init() {
    L_DEBUG(_u("Initializing virtual gamepad with {0} buttons").format(button_count_));

    ui_stage_ = window_.new_ui_stage(); //Create a UI stage to hold the controller buttons

    uint32_t button_size = int(float(window_.width() / 10.0));
    uint32_t dpad_margin = int(float(window_.width() * (5.0 / 640.0)));
    uint32_t area_width = int(float(window_.width() * (200.0 / 640.0)));
    uint32_t outside_padding = int(float(window_.width() * (10.0 / 640.0)));

    auto stage = window_.ui_stage(ui_stage_);
    stage->load_rml_from_string(layout.format(button_size, dpad_margin, area_width, outside_padding));

    if(this->directions_ == VIRTUAL_DPAD_DIRECTIONS_TWO) {
        stage->$(".dpad_two").css("display", "inline-block");

        stage->$(".dpad_left").set_event_callback("mousedown", [=]() -> bool {
            signal_hat_changed_(HAT_POSITION_LEFT);
            return true;
        });

        stage->$(".dpad_left").set_event_callback("mouseup", [=]() -> bool {
            signal_hat_changed_(HAT_POSITION_CENTERED);
            return true;
        });

        stage->$(".dpad_right").set_event_callback("mousedown", [=]() -> bool {
            signal_hat_changed_(HAT_POSITION_RIGHT);
            return true;
        });

        stage->$(".dpad_right").set_event_callback("mouseup", [=]() -> bool {
            signal_hat_changed_(HAT_POSITION_CENTERED);
            return true;
        });
    }

    //Make the buttons visible that need to be
    for(int i = 1; i < button_count_ + 1; ++i) {
        unicode klass = _u(".button_{0}").format(humanize(i));
        stage->$(klass).css("display", "inline-block");

        //Forward the events from the UI on to the signals
        stage->$(klass).set_event_callback("mousedown", [=]() -> bool {
            signal_button_down_(i - 1);
            return true;
        });
        stage->$(klass).set_event_callback("mouseup", [=]() -> bool {
            signal_button_up_(i - 1);
            return true;
        });
    }

    camera_id_ = window_.new_camera_with_orthographic_projection();

    //Finally add to the render sequence, give a ridiculously high priority
    window_.render_sequence()->new_pipeline(ui_stage_, camera_id_, kglt::ViewportID(), kglt::TextureID(), 100000);

    return true;
}

void VirtualGamepad::cleanup() {
    L_DEBUG("Destroying virtual gamepad");

    window_.delete_camera(camera_id_);
    window_.delete_ui_stage(ui_stage_);
}

void VirtualGamepad::flip() {

}

}
