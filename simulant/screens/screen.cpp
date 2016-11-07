#include "screen.h"
#include "../window_base.h"

namespace smlt {

ScreenBase::ScreenBase(WindowBase &window, const unicode& name):
    window_(&window),
    name_(name) {

}

ScreenBase::~ScreenBase() {

}

void ScreenBase::load() {
    if(is_loaded_) {
        return;
    }

    do_load();
    is_loaded_ = true;
}

void ScreenBase::unload() {
    if(!is_loaded_) {
        return;
    }

    do_unload();
    is_loaded_ = false;
}

void ScreenBase::activate() {
    do_activate();
}

void ScreenBase::deactivate() {
    do_deactivate();
}

void ScreenBase::step(double dt) {
    do_step(dt);
}

PipelineID ScreenBase::prepare_basic_scene(StageID& new_stage, CameraID& new_camera, AvailablePartitioner partitioner) {
    new_stage = window->new_stage(partitioner);
    new_camera = window->new_camera();
    return window->render(new_stage, new_camera).with_clear();
}

std::pair<PipelineID, PipelineID> ScreenBase::prepare_basic_scene_with_overlay(
    StageID& new_stage, CameraID& new_camera,
    OverlayID& new_ui, CameraID& new_ui_camera) {

    PipelineID first = prepare_basic_scene(new_stage, new_camera);

    new_ui = window->new_overlay();
    new_ui_camera = window->new_camera_for_ui();

    PipelineID second = window->render(new_ui, new_ui_camera);

    return std::make_pair(first, second);
}

}

