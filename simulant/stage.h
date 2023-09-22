/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "nodes/stage_node.h"

namespace smlt {

struct StageParams {};

/** A stage is a container node, it has no mesh of its own
 *  it exists solely to umbrella other nodes underneath it.
 *  It's useful to use stages to group things for rendering by render pipelines
*/

class Stage;

template<>
struct stage_node_traits<Stage> {
    const static StageNodeType node_type = STAGE_NODE_TYPE_STAGE;
    typedef StageParams params_type;
};

class Stage : public StageNode {
public:
    Stage(Scene* owner);
private:
    bool on_create(void *params) override {
        _S_UNUSED(params);
        return true;
    }
};


}

