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

#include "scene.h"
#include "../stage.h"
#include "../window_base.h"

namespace smlt {

SceneBase::SceneBase(WindowBase &window):
    window_(&window) {

}

SceneBase::~SceneBase() {

}

void SceneBase::_call_load() {
    if(is_loaded_) {
        return;
    }

    pre_load();
    load();

    is_loaded_ = true;
}

void SceneBase::_call_unload() {
    if(!is_loaded_) {
        return;
    }

    unload();
    post_unload();

    is_loaded_ = false;
}

void SceneBase::_call_activate() {
    activate();
}

void SceneBase::_call_deactivate() {
    deactivate();
}

PipelineID SceneBase::prepare_basic_scene(StageID& new_stage, CameraID& new_camera, AvailablePartitioner partitioner) {
    new_stage = window->new_stage(partitioner);
    new_camera = new_stage.fetch()->new_camera();
    return window->render(new_stage, new_camera).with_clear();
}

}

