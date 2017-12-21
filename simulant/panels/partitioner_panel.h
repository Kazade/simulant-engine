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

#pragma once

#include <unordered_map>
#include "panel.h"
#include "../types.h"

namespace smlt {

class Window;

class PartitionerPanel : public Panel {
public:
    PartitionerPanel(Window* window);

private:
    Window* window_ = nullptr;

    void do_activate() override;
    void do_deactivate() override;
    void initialize();
    bool initialized_ = false;

    std::unordered_map<StageID, ActorPtr> debug_actors_;
};

}
