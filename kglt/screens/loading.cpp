#include "../scene.h"
#include "../stage.h"
#include "../procedural/geom_factory.h"
#include "../render_sequence.h"

#include "loading.h"

namespace kglt {
namespace screens {

Loading::Loading(Scene &scene):
    scene_(scene),
    is_active_(false) {

    //Create a stage
    stage_ = scene.new_stage();

    scene_.stage(stage_).geom_factory().new_cube(1.0);

    //Create an orthographic camera
    camera_ = scene.new_camera();

    //Create an inactive pipeline
    pipeline_ = scene.render_sequence().new_pipeline(
        stage_,
        camera_
    );

    scene.render_sequence().pipeline(pipeline_).deactivate();
}

Loading::~Loading() {
    //Clean up
    scene_.render_sequence().delete_pipeline(pipeline_);
    scene_.delete_stage(stage_);
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
