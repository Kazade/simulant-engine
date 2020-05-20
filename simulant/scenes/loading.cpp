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

#include "../stage.h"
#include "../compositor.h"
#include "../nodes/actor.h"
#include "../nodes/camera.h"
#include "../window.h"
#include "../nodes/ui/ui_manager.h"
#include "../nodes/ui/label.h"

#include "loading.h"

namespace smlt {
namespace scenes {


void Loading::load() {
    //Create a stage
    stage_ = window->new_stage();

    progress_bar_ = stage_->ui->new_widget_as_progress_bar();
    progress_bar_->resize(window->width() * 0.5f, 8);
    progress_bar_->set_anchor_point(0.5f, 0.5f);
    progress_bar_->move_to(window->coordinate_from_normalized(0.5, 0.5));
    progress_bar_->set_pulse_step(progress_bar_->requested_width());

    auto label = stage_->ui->new_widget_as_label("LOADING");
    label->set_anchor_point(0.5f, 0.5f);
    label->move_to(window->coordinate_from_normalized(0.5, 0.55));
    label->set_background_colour(smlt::Colour::NONE);

    //Create an orthographic camera
    camera_ = stage_->new_camera();

    camera_->set_orthographic_projection(
        0, window->width(), 0, window->height()
    );

    //Create an inactive pipeline
    pipeline_ = compositor->render(stage_, camera_);
    pipeline_->deactivate();
}

void Loading::unload() {
    //Clean up
    pipeline_->deactivate();
    window->destroy_stage(stage_->id());
}

void Loading::activate() {
    pipeline_->activate();
}

void Loading::deactivate() {
    //Deactivate the loading pipeline
    pipeline_->deactivate();
}

}
}
