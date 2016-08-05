#include "../stage.h"
#include "../render_sequence.h"
#include "../actor.h"
#include "../ui_stage.h"
#include "../camera.h"
#include "../window_base.h"

#include "loading.h"

namespace kglt {
namespace screens {


void Loading::do_load() {
    //Create a stage
    stage_ = window->new_ui_stage();

    auto stage = window->ui_stage(stage_);

    stage->set_styles(R"X(
        body {
            font-family: "Ubuntu";
            display: block;
            height: 100%;
            width: 100%;
            background-color: #000000FF;
            vertical-align: middle;
        }
        p, div {
            display: block;
        }

        .thing {
            position: absolute;

            color: white;
            font-size: 16px;
            width: 100%;
            height: 100%;

            top: 48%;
            text-align: center;
        }

    )X");

    auto $window = stage->append("<window>");
    $window.append("<row>").append("<label>").text("Loading");
    $window.append("<row>").append("<progress>");

    stage->$("label").add_class("thing");

    //Create an orthographic camera
    camera_ = window->new_camera();

    window->camera(camera_)->set_orthographic_projection(
        0, window->width(), window->height(), 0, -1.0, 1.0
    );

    //Create an inactive pipeline
    pipeline_ = window->render(stage_, camera_);
    window->disable_pipeline(pipeline_);
}

void Loading::do_unload() {
    //Clean up
    window->delete_pipeline(pipeline_);
    window->delete_ui_stage(stage_);
    window->delete_camera(camera_);
}

void Loading::do_activate() {
    window->enable_pipeline(pipeline_);
}

void Loading::do_deactivate() {
    //Deactivate the loading pipeline
    window->disable_pipeline(pipeline_);
}

}
}
