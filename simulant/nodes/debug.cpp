//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation, either version 3 of the License, or (at
//     your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "debug.h"
#include "actor.h"

#include "../application.h"
#include "../compat.h"
#include "../macros.h"
#include "../stage.h"
#include "../time_keeper.h"
#include "../utils/random.h"
#include "../window.h"

namespace smlt {

Debug::Debug(Scene* owner) :
    StageNode(owner, STAGE_NODE_TYPE_DEBUG) {

    frame_connection_ = get_app()->signal_frame_started().connect(
        std::bind(&Debug::reset, this));
}

Debug::~Debug() {
    // Make sure we disconnect otherwise crashes happen
    frame_connection_.disconnect();
}

void Debug::reset() {
    auto dt = get_app()->time_keeper->delta_time();

    for(auto it = elements_.begin(); it != elements_.end();) {
        auto& element = (*it);
        element.time_since_created += dt;
        if(element.time_since_created >= element.duration) {
            it = elements_.erase(it);
        } else {
            ++it;
        }
    }
}

void Debug::push_line(SubMeshPtr submesh, const Vec3& start, const Vec3& end,
                      const Color& color) {

    auto p = (end - start).perpendicular().normalized();

    Vec3 a = start + p;
    Vec3 b = start - p;
    Vec3 c = end + p;
    Vec3 d = end - p;

    auto c_ = mesh_->vertex_data->count();
    mesh_->vertex_data->position(a);
    mesh_->vertex_data->diffuse(color);
    mesh_->vertex_data->move_next();

    mesh_->vertex_data->position(b);
    mesh_->vertex_data->diffuse(color);
    mesh_->vertex_data->move_next();

    mesh_->vertex_data->position(c);
    mesh_->vertex_data->diffuse(color);
    mesh_->vertex_data->move_next();

    mesh_->vertex_data->position(d);
    mesh_->vertex_data->diffuse(color);
    mesh_->vertex_data->move_next();

    auto i = c_;
    submesh->index_data->index(i);
    submesh->index_data->index(i + 1);
    submesh->index_data->index(i + 2);

    submesh->index_data->index(i + 1);
    submesh->index_data->index(i + 3);
    submesh->index_data->index(i + 2);

    submesh->index_data->done();
}

void Debug::push_point(SubMeshPtr submesh, const Vec3& position,
                       const Color& color, float size) {

    /* Although this is supposed to be a point, what we actually do is build
     * an axis-aligned box to represent the point. This is because many
     * platforms don't support point drawing */

    float hs = size / 2.0f;
    auto c = mesh_->vertex_data->count();

    for(int i = -1; i <= 1; i += 2) {
        for(int j = -1; j <= 1; j += 2) {
            for(int k = -1; k <= 1; k += 2) {
                mesh_->vertex_data->position(position +
                                             Vec3(i * hs, j * hs, k * hs));
                mesh_->vertex_data->diffuse(color);
                mesh_->vertex_data->move_next();
            }
        }
    }

    auto i = c;
    submesh->index_data->index(i);
    submesh->index_data->index(i + 1);
    submesh->index_data->index(i + 2);

    submesh->index_data->index(i);
    submesh->index_data->index(i + 2);
    submesh->index_data->index(i + 3);

    submesh->index_data->index(i);
    submesh->index_data->index(i + 4);
    submesh->index_data->index(i + 5);

    submesh->index_data->index(i);
    submesh->index_data->index(i + 5);
    submesh->index_data->index(i + 1);

    submesh->index_data->index(i + 1);
    submesh->index_data->index(i + 5);
    submesh->index_data->index(i + 6);

    submesh->index_data->index(i + 1);
    submesh->index_data->index(i + 6);
    submesh->index_data->index(i + 2);

    submesh->index_data->index(i + 2);
    submesh->index_data->index(i + 6);
    submesh->index_data->index(i + 7);

    submesh->index_data->index(i + 2);
    submesh->index_data->index(i + 7);
    submesh->index_data->index(i + 3);

    submesh->index_data->index(i + 3);
    submesh->index_data->index(i + 7);
    submesh->index_data->index(i + 4);

    submesh->index_data->index(i + 3);
    submesh->index_data->index(i + 4);
    submesh->index_data->index(i + 0);

    submesh->index_data->index(i + 4);
    submesh->index_data->index(i + 7);
    submesh->index_data->index(i + 6);

    submesh->index_data->index(i + 4);
    submesh->index_data->index(i + 6);
    submesh->index_data->index(i + 5);
}

void Debug::build_mesh() {
    mesh_->vertex_data->clear();
    without_depth_->index_data->clear();
    with_depth_->index_data->clear();

    for(auto& element: elements_) {
        if(element.type == DET_LINE) {
            push_line((element.depth_test) ? with_depth_ : without_depth_,
                      element.points[0], element.points[1], element.color);

        } else {
            push_point((element.depth_test) ? with_depth_ : without_depth_,
                       element.points[0], element.color, element.size);
        }
    }

    mesh_->vertex_data->done();
    without_depth_->index_data->done();
    with_depth_->index_data->done();
}

void Debug::set_transform(const Mat4& mat) {
    transform_ = mat;
}

Mat4 Debug::transform() const {
    return transform_;
}

bool Debug::on_init() {
    mesh_ =
        scene->assets->create_mesh(VertexSpecification::POSITION_AND_DIFFUSE);

    // Don't GC the material, if there are no debug lines then it won't be
    // attached to the mesh
    material_ = scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);

    material_->set_cull_mode(CULL_MODE_NONE);

    // Never write to the depth buffer with debug stuff
    material_->set_depth_write_enabled(false);

    material_no_depth_ =
        scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);
    material_no_depth_->set_depth_write_enabled(false);
    material_no_depth_->set_depth_test_enabled(false);
    material_no_depth_->set_cull_mode(CULL_MODE_NONE);

    with_depth_ = mesh_->create_submesh(
        "with_depth", material_, INDEX_TYPE_16_BIT, MESH_ARRANGEMENT_TRIANGLES);
    without_depth_ =
        mesh_->create_submesh("without_depth", material_no_depth_,
                              INDEX_TYPE_16_BIT, MESH_ARRANGEMENT_TRIANGLES);

    return true;
}

void Debug::do_generate_renderables(batcher::RenderQueue* render_queue,
                                    const Camera* camera, const Viewport*,
                                    const DetailLevel detail_level) {

    if(elements_.empty()) {
        return;
    }

    build_mesh();

    if(mesh_->vertex_data->count() == 0) {
        return;
    }

    _S_UNUSED(camera);
    _S_UNUSED(detail_level);

    for(auto& submesh: mesh_->each_submesh()) {
        Renderable new_renderable;

        // Debug element positions are always absolute
        new_renderable.final_transformation = Mat4();
        new_renderable.render_priority = render_priority();
        new_renderable.is_visible = is_visible();
        new_renderable.arrangement = submesh->arrangement();
        new_renderable.vertex_data = mesh_->vertex_data.get();
        new_renderable.index_data = submesh->index_data.get();
        new_renderable.index_element_count = submesh->index_data->count();
        new_renderable.vertex_ranges = submesh->vertex_ranges();
        new_renderable.vertex_range_count = submesh->vertex_range_count();
        new_renderable.material = submesh->material().get();
        new_renderable.center = Vec3();

        render_queue->insert_renderable(std::move(new_renderable));
    }
}

void Debug::set_point_size(float ps) {
    current_point_size_ = ps;
}

float Debug::point_size() const {
    return current_point_size_;
}

void Debug::draw_line(const Vec3& start, const Vec3& end, const Color& color,
                      float duration, bool depth_test) {

    DebugElement element;
    element.color = color;
    element.duration = duration;
    element.depth_test = depth_test;
    element.points[0] = start.transformed_by(transform_);
    element.points[1] = end.transformed_by(transform_);
    element.size = 0.25f;
    element.type = DebugElementType::DET_LINE;
    elements_.push_back(element);
}

void Debug::draw_ray(const Vec3& start, const Vec3& dir, const Color& color,
                     float duration, bool depth_test) {
    draw_line(start, start + dir, color, duration, depth_test);
}

void Debug::draw_point(const Vec3& position, const Color& color, float duration,
                       bool depth_test) {
    DebugElement element;
    element.color = color;
    element.duration = duration;
    element.depth_test = depth_test;
    element.points[0] = position.transformed_by(transform_);
    element.type = DebugElementType::DET_POINT;
    element.size = current_point_size_;
    elements_.push_back(element);
}

} // namespace smlt
