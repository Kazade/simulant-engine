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

#include "stage_node.h"
#include "../types.h"
#include "../generic/managed.h"
#include "../meshes/mesh.h"


namespace smlt {

struct DebugParams {};

class Debug : public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_DEBUG, DebugParams);

    Debug(Scene* owner);
    virtual ~Debug();

    void draw_ray(
        const Vec3& start,
        const Vec3& dir,
        const Color& color=Color::WHITE,
        double duration=0.0,
        bool depth_test=true
    );


    void draw_line(
        const Vec3& start,
        const Vec3& end,
        const Color& color=Color::WHITE,
        double duration=0.0,
        bool depth_test=true
    );

    void draw_point(
        const Vec3& position,
        const Color& color=Color::WHITE,
        double duration=0.0,
        bool depth_test=true
    );

    bool on_init() override;

    void set_point_size(float ps);

    float point_size() const;

    void on_update(float dt);

    void set_transform(const Mat4& mat);
    Mat4 transform() const;

private:
    Mat4 transform_;

    void frame_finished();

    bool initialized_ = false;

    void initialize_actor();

    enum DebugElementType {
        DET_LINE,
        DET_POINT
    };

    struct DebugElement {
        float time_since_created = 0.0;
        DebugElementType type = DET_LINE;
        Color color = Color::WHITE;
        bool depth_test = true;
        float duration = 0.0;

        smlt::Vec3 points[2]; // For lines, or the first one for points
        float size; // Diameter for spheres + points
    };

    std::list<DebugElement> elements_;

    SubMesh* lines_without_depth_ = nullptr;
    SubMesh* lines_with_depth_ = nullptr;
    SubMesh* points_without_depth_ = nullptr;
    SubMesh* points_with_depth_ = nullptr;

    MeshPtr mesh_;
    ActorPtr actor_ = nullptr;
    MaterialPtr material_;
    MaterialPtr material_no_depth_;
    float current_point_size_ = 0.001f;

    sig::Connection frame_finished_connection_;

    bool on_create(void *params) override {
        _S_UNUSED(params);
        return true;
    }
};

}
