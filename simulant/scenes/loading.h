/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOADING_H
#define LOADING_H

#include <vector>

#include "../generic/property.h"
#include "../generic/managed.h"
#include "../types.h"
#include "scene.h"
#include "../nodes/ui/progress_bar.h"

namespace smlt {


namespace scenes {

class Loading:
    public Scene<Loading> {

public:
    Loading(Window* window):
        Scene<Loading>(window) {}

    Property<Loading, ui::ProgressBar> progress_bar = {this, &Loading::progress_bar_};

private:
    void activate() override;
    void deactivate() override;

    void load() override;
    void unload() override;

    StagePtr stage_;
    CameraPtr camera_;
    PipelineID pipeline_;

    ui::ProgressBar* progress_bar_ = nullptr;
};

}
}

#endif // LOADING_H
