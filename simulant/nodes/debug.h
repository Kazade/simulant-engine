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

#include "../generic/managed.h"
#include "../meshes/mesh.h"
#include "../types.h"
#include "stage_node.h"

namespace smlt {

class Debug: public StageNode, public HasMutableRenderPriority {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_DEBUG, "debug");

    Debug(Scene* owner);
    virtual ~Debug();

    void draw_ray(const Vec3& start, const Vec3& dir,
                  const Color& color = Color::white(), float duration = 0.0f,
                  bool depth_test = true);

    void draw_line(const Vec3& start, const Vec3& end,
                   const Color& color = Color::white(), float duration = 0.0f,
                   bool depth_test = true);

    void draw_point(const Vec3& position, const Color& color = Color::white(),
                    float duration = 0.0, bool depth_test = true);

    bool on_init() override;

    void set_point_size(float ps);
    float point_size() const;

    void set_line_width(float size);
    float line_width() const;

private:
    void do_generate_renderables(batcher::RenderQueue* render_queue,
                                 const Camera* camera, const Viewport*,
                                 const DetailLevel detail_level) override;

    void reset();
    void build_mesh(const Camera* camera);

    void push_line(SubMeshPtr& submesh, const Vec3& start, const Vec3& end,
                   const Color& color, const Vec3& camera_pos);

    void push_point(SubMeshPtr& submesh, const Vec3& position,
                    const Color& color, float size, const Vec3& up,
                    const Vec3& right);

    enum DebugElementType {
        DET_LINE,
        DET_POINT
    };

    struct DebugElement {
        float time_since_created = 0.0;
        DebugElementType type = DET_LINE;
        Color color = Color::white();
        bool depth_test = true;
        float duration = 0.0f;

        smlt::Vec3 points[2]; // For lines, or the first one for points
        float size;           // Diameter for spheres + points
    };

    std::vector<DebugElement> elements_;

    SubMesh* without_depth_ = nullptr;
    SubMesh* with_depth_ = nullptr;

    MeshPtr mesh_ = nullptr;
    ActorPtr actor_ = nullptr;
    MaterialPtr material_;
    MaterialPtr material_no_depth_;

    float current_point_size_ = 0.01f;
    float current_line_width_ = 0.01f;

    sig::Connection frame_connection_;

    bool on_create(Params params) override {
        _S_UNUSED(params);
        return true;
    }
};

} // namespace smlt
