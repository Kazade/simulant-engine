#include "../stage.h"
#include "../procedural/geom_factory.h"
#include "../render_sequence.h"
#include "../actor.h"
#include "../ui_stage.h"
#include "../camera.h"
#include "../window_base.h"

#include "loading.h"

namespace kglt {
namespace screens {

Loading::Loading(WindowBase& window):
    window_(window),
    is_active_(false) {

}

bool Loading::init() {
    //Create a stage
    stage_ = window().new_ui_stage();

    auto stage = window().ui_stage(stage_);

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

    stage->append("<p>").text("Loading");
    stage->$("p").add_class("thing");

    //Create an orthographic camera
    camera_ = window().new_camera();

    window().camera(camera_)->set_orthographic_projection(
        0, window().width(), window().height(), 0
    );

    //Create an inactive pipeline
    pipeline_ = window().render_sequence()->new_pipeline(
        stage_,
        camera_
    );

    window().render_sequence()->pipeline(pipeline_).deactivate();
    return true;
}

void Loading::cleanup() {
    //Clean up
    window().render_sequence()->delete_pipeline(pipeline_);
    window().delete_ui_stage(stage_);
    window().delete_camera(camera_);
}

void Loading::activate() {
    is_active_ = true;

    stashed_pipelines_ = window().render_sequence()->active_pipelines();

    window().render_sequence()->deactivate_all_pipelines();
    window().render_sequence()->pipeline(pipeline_).activate();
}

void Loading::deactivate() {
    is_active_ = false;

    //Deactivate the loading pipeline
    window().render_sequence()->pipeline(pipeline_).deactivate();

    //Activate the stashed pipelines
    window().render_sequence()->activate_pipelines(stashed_pipelines_);

    stashed_pipelines_.clear();
}

void Loading::update(float dt) {
    if(!is_active()) return;
}

bool Loading::is_active() const {
    return is_active_;
}

}
}
