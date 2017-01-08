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
#include "../actor.h"
#include "../overlay.h"
#include "../camera.h"
#include "../window_base.h"

#include "loading.h"

namespace smlt {
namespace screens {


void Loading::do_load() {
    //Create a stage
    stage_ = window->new_overlay();

    auto stage = window->overlay(stage_);

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


    stage->append_row().append_label("Loading");
    stage->append_row().append_progress_bar();

    stage->find("label").add_class("thing");

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
    window->delete_overlay(stage_);
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
