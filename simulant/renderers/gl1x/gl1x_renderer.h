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

#include "../renderer.h"
#include "../gl_renderer.h"

namespace smlt {

class GL1XRenderer:
    public GLRenderer {

public:
    friend class GL1RenderQueueVisitor;

    GL1XRenderer(Window* window);

    batcher::RenderGroupKey prepare_render_group(batcher::RenderGroup* group,
        const Renderable *renderable,
        const MaterialPass *material_pass,
        const uint8_t pass_number,
        const bool is_blended,
        const float distance_to_camera) override;

    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera) override;

    void init_context() override;

    std::string name() const override {
        return "gl1x";
    }

private:
    virtual void on_pre_render() override;

    TexturePtr normalization_cube_map_;

    void init_normalization_map();

    virtual std::shared_ptr<VertexBuffer> prepare_vertex_data(
        MeshArrangement arrangement, const VertexData* vertex_data,
        const IndexData* index_data, const VertexRangeList* ranges) override;

    VertexFormat on_native_vertex_format(VertexFormat hint) override {
        return VertexFormatBuilder()
            .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
                 VERTEX_ATTR_TYPE_FLOAT)
            .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
                 VERTEX_ATTR_TYPE_FLOAT)
            .add(VERTEX_ATTR_NAME_COLOR, VERTEX_ATTR_ARRANGEMENT_BGRA,
                 VERTEX_ATTR_TYPE_UNSIGNED_BYTE)
            .add(VERTEX_ATTR_NAME_NORMAL, VERTEX_ATTR_ARRANGEMENT_XYZ,
                 VERTEX_ATTR_TYPE_FLOAT)
            .build();
    }
};

}
