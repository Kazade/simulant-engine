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

#include "compat.h"
#include "stage.h"
#include "debug.h"
#include "nodes/actor.h"
#include "random.h"
#include "core.h"
#include "macros.h"

namespace smlt {

Debug::Debug(Stage &stage):
    stage_(stage) {

    frame_finished_connection_ = stage_.window->signal_frame_finished().connect(
        std::bind(&Debug::frame_finished, this)
    );
}

Debug::~Debug() {
    // Make sure we disconnect otherwise crashes happen
    frame_finished_connection_.disconnect();

    if(mesh_) {
        auto mesh = mesh_.fetch();
        if(mesh) {
            // Enable GC
            mesh->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);
        }

        mesh_ = MeshID();
    }

    if(actor_) {
        actor_->destroy();
    }
}

void Debug::frame_finished() {
    auto dt = stage_.window->time_keeper->delta_time();

    for(auto it = elements_.begin(); it != elements_.end(); ++it) {
        auto& element = (*it);
        element.time_since_created += dt;
        if(element.time_since_created >= element.duration) {
            it = elements_.erase(it);
        }
    }
}

void Debug::update(float dt) {
    _S_UNUSED(dt);

    auto mesh = mesh_.fetch();

    mesh->vertex_data->clear();
    lines_without_depth_->index_data->clear();
    lines_with_depth_->index_data->clear();
    points_without_depth_->index_data->clear();
    points_with_depth_->index_data->clear();

    for(auto& element: elements_) {
        if(element.type == DET_LINE) {
            auto& array = (element.depth_test) ? lines_with_depth_->index_data : lines_without_depth_->index_data;
            auto i = mesh->vertex_data->count();
            mesh->vertex_data->position(element.points[0]);
            mesh->vertex_data->diffuse(element.colour);
            mesh->vertex_data->move_next();

            mesh->vertex_data->position(element.points[1]);
            mesh->vertex_data->diffuse(element.colour);
            mesh->vertex_data->move_next();

            array->index(i);
            array->index(i + 1);
        } else {
            /* HACKITY HACKITY HACKITY HACK */
            /*
             * Need to support points sprites, and use those... or at least make these billboard quads!
             * (Simulant issue #133)
            */

            float hs = element.size / 2.0f;
            auto& array = (element.depth_test) ? points_with_depth_->index_data : points_without_depth_->index_data;
            auto i = mesh->vertex_data->count();
            mesh->vertex_data->position(element.points[0] + smlt::Vec3(-hs, hs, 0));
            mesh->vertex_data->diffuse(element.colour);
            mesh->vertex_data->move_next();

            mesh->vertex_data->position(element.points[0] + smlt::Vec3(-hs, -hs, 0));
            mesh->vertex_data->diffuse(element.colour);
            mesh->vertex_data->move_next();

            mesh->vertex_data->position(element.points[0] + smlt::Vec3(hs, -hs, 0));
            mesh->vertex_data->diffuse(element.colour);
            mesh->vertex_data->move_next();

            mesh->vertex_data->position(element.points[0] + smlt::Vec3(hs, hs, 0));
            mesh->vertex_data->diffuse(element.colour);
            mesh->vertex_data->move_next();

            array->index(i);
            array->index(i + 1);
            array->index(i + 2);

            array->index(i);
            array->index(i + 2);
            array->index(i + 3);
        }
    }

    mesh->vertex_data->done();
    lines_without_depth_->index_data->done();
    lines_with_depth_->index_data->done();
    points_without_depth_->index_data->done();
    points_with_depth_->index_data->done();
}

void Debug::set_transform(const Mat4& mat) {
    transform_ = mat;
}

Mat4 Debug::transform() const {
    return transform_;
}

void Debug::initialize_actor() {
    /*
     * We don't initialize the actor until first use to avoid unnecessary
     * clutter in the render queue in release setups. We could potentially
     * move the asset loading here too but that would result in a stutter on
     * first draw as files are loaded.
     */

    if(initialized_) {
        return;
    }

    actor_ = stage_.new_actor_with_mesh(
        mesh_
    );

    // Important. Debug stuff shouldn't be culled
    actor_->set_cullable(false);

    // Always render debug stuff last, and don't cull
    actor_->set_render_priority(RENDER_PRIORITY_ABSOLUTE_FOREGROUND);

    initialized_ = true;
}

bool Debug::init() {
    mesh_ = stage_.assets->new_mesh(VertexSpecification::POSITION_AND_DIFFUSE, GARBAGE_COLLECT_NEVER);

    //Don't GC the material, if there are no debug lines then it won't be attached to the mesh
    material_ = stage_.assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY, GARBAGE_COLLECT_NEVER);

    auto mat = material_.fetch();

    mat->set_cull_mode(CULL_MODE_NONE);

    // Never write to the depth buffer with debug stuff
    mat->set_depth_write_enabled(false);

    material_no_depth_ = stage_.assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY, GARBAGE_COLLECT_NEVER);
    material_no_depth_.fetch()->set_depth_write_enabled(false);
    material_no_depth_.fetch()->set_depth_test_enabled(false);
    material_no_depth_.fetch()->set_cull_mode(CULL_MODE_NONE);

    auto mesh = mesh_.fetch();

    lines_with_depth_ = mesh->new_submesh_with_material("lines_with_depth", material_, MESH_ARRANGEMENT_LINES);
    lines_without_depth_ = mesh->new_submesh_with_material("lines_without_depth", material_no_depth_, MESH_ARRANGEMENT_LINES);
    points_with_depth_ = mesh->new_submesh_with_material("points_with_depth", material_, MESH_ARRANGEMENT_TRIANGLES);
    points_without_depth_ = mesh->new_submesh_with_material("points_without_depth", material_no_depth_, MESH_ARRANGEMENT_TRIANGLES);

    return true;
}

void Debug::set_point_size(float ps) {
    current_point_size_ = ps;
}

float Debug::point_size() const {
    return current_point_size_;
}

void Debug::draw_line(const Vec3 &start, const Vec3 &end, const Colour &colour, double duration, bool depth_test) {
    initialize_actor();

    DebugElement element;
    element.colour = colour;
    element.duration = duration;
    element.depth_test = depth_test;
    element.points[0] = start.transformed_by(transform_);
    element.points[1] = end.transformed_by(transform_);
    element.size = 0.25f;
    element.type = DebugElementType::DET_LINE;
    elements_.push_back(element);
}

void Debug::draw_ray(const Vec3 &start, const Vec3 &dir, const Colour &colour, double duration, bool depth_test) {
    initialize_actor();

    draw_line(start, start+dir, colour, duration, depth_test);
}

void Debug::draw_point(const Vec3 &position, const Colour &colour, double duration, bool depth_test) {
    initialize_actor();

    DebugElement element;
    element.colour = colour;
    element.duration = duration;
    element.depth_test = depth_test;
    element.points[0] = position.transformed_by(transform_);
    element.type = DebugElementType::DET_POINT;
    element.size = current_point_size_;
    elements_.push_back(element);
}

}
