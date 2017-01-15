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

#include "../stage.h"
#include "../render_sequence.h"
#include "../nodes/actor.h"
#include "../camera.h"
#include "../window_base.h"
#include "../nodes/ui/ui_manager.h"

#include "loading.h"

namespace smlt {
namespace screens {


void Loading::do_load() {
    //Create a stage
    stage_ = window->new_stage();

    auto stage = stage_.fetch();
    auto progress_bar = stage->ui->new_widget_as_progress_bar().fetch();
    progress_bar->move_to(window->coordinate_from_normalized(0.5, 0.5));

    //Create an orthographic camera
    camera_ = window->new_camera();

    window->camera(camera_)->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    //Create an inactive pipeline
    pipeline_ = window->render(stage_, camera_);
    window->disable_pipeline(pipeline_);
}

void Loading::do_unload() {
    //Clean up
    window->delete_pipeline(pipeline_);
    window->delete_stage(stage_);
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
