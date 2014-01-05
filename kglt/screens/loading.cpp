#include "../scene.h"
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

Loading::Loading(Scene &scene):
    scene_(scene),
    is_active_(false) {

}

bool Loading::init() {
    //Create a stage
    stage_ = scene_.new_ui_stage();

    auto stage = scene_.ui_stage(stage_);

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
    camera_ = scene_.new_camera();

    scene_.camera(camera_).set_orthographic_projection(
        0, scene_.window().width(), scene_.window().height(), 0
    );

    //Create an inactive pipeline
    pipeline_ = scene_.render_sequence().new_pipeline(
        stage_,
        camera_
    );

    scene_.render_sequence().pipeline(pipeline_).deactivate();
    return true;
}

void Loading::cleanup() {
    //Clean up
    scene_.render_sequence().delete_pipeline(pipeline_);
    scene_.delete_ui_stage(stage_);
    scene_.delete_camera(camera_);
}

void Loading::activate() {
    is_active_ = true;

    stashed_pipelines_ = scene_.render_sequence().active_pipelines();

    scene_.render_sequence().deactivate_all_pipelines();
    scene_.render_sequence().pipeline(pipeline_).activate();
}

void Loading::deactivate() {
    is_active_ = false;

    //Deactivate the loading pipeline
    scene_.render_sequence().pipeline(pipeline_).deactivate();

    //Activate the stashed pipelines
    scene_.render_sequence().activate_pipelines(stashed_pipelines_);

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
