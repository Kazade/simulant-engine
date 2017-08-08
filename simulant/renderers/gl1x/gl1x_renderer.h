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

#include "../renderer.h"
#include "gl1x_buffer_manager.h"

namespace smlt {


class GL1XRenderer : public Renderer {
public:
    GL1XRenderer(Window* window);

    batcher::RenderGroup new_render_group(Renderable *renderable, MaterialPass *material_pass);
    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera);

    void init_context();

private:
    std::unique_ptr<HardwareBufferManager> buffer_manager_;

    HardwareBufferManager* _get_buffer_manager() const {
        /*
         * The GL1 renderer doesn't use hardware buffers for vertex/index data
         */
        return buffer_manager_.get();
    }
};

}
