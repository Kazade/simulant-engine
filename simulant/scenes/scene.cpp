//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "scene.h"
#include "../compositor.h"
#include "../stage.h"
#include "../core.h"
#include "../pipeline.h"

namespace smlt {

SceneBase::SceneBase(Core *core):
    core_(core),
    input_(core->input.get()),
    app_(core->application),
    compositor_(core->compositor) {

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

    _call_deactivate();

    is_loaded_ = false;
    unload();
    post_unload();
}

void SceneBase::_call_activate() {
    if(is_active_) {
        return;
    }

    activate();
    is_active_ = true;

    for(auto name: linked_pipelines_) {
        compositor->find_pipeline(name)->activate();
    }
}

void SceneBase::_call_deactivate() {
    if(!is_active_) {
        return;
    }

    for(auto name: linked_pipelines_) {
        compositor->find_pipeline(name)->deactivate();
    }

    deactivate();
    is_active_ = false;
}

void SceneBase::link_pipeline(const std::string &name) {
    linked_pipelines_.insert(name);
}

void SceneBase::unlink_pipeline(const std::string &name) {
    linked_pipelines_.insert(name);
}

void SceneBase::link_pipeline(PipelinePtr pipeline) {
    link_pipeline(pipeline->name());
}

void SceneBase::unlink_pipeline(PipelinePtr pipeline) {
    unlink_pipeline(pipeline->name());
}


}

