/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
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
#include "simulant/utils/construction_args.h"

namespace smlt {

struct StageParams {};

/** A stage is a container node, it has no mesh of its own
 *  it exists solely to umbrella other nodes underneath it.
 *  It's useful to use stages to group things for rendering by render pipelines
 */

class Stage;

class Stage: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_STAGE);

    Stage(Scene* owner);

    const AABB& aabb() const override {
        static AABB aabb;
        return aabb;
    }

private:
    bool on_create(const ConstructionArgs& params) override {
        _S_UNUSED(params);
        return true;
    }

    void do_generate_renderables(batcher::RenderQueue*, const Camera*,
                                 const Viewport*, DetailLevel) override {}
};

} // namespace smlt
