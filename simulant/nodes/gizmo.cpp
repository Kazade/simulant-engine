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

#include "gizmo.h"

#include <algorithm>
#include <cmath>

#include "../asset_manager.h"
#include "../assets/material.h"
#include "../interfaces.h"
#include "../math/degrees.h"
#include "../math/radians.h"
#include "../math/ray.h"
#include "../meshes/mesh.h"
#include "../scenes/scene.h"
#include "../viewport.h"
#include "../window.h"
#include "actor.h"
#include "camera.h"

namespace smlt {

namespace {
const float SHAFT_LENGTH = 1.3f;
const float SHAFT_DIAMETER = 0.07f;
const float TIP_LENGTH = 0.35f;
const float TIP_DIAMETER = 0.16f;
const float RING_MAJOR_RADIUS = 1.5f;
const float RING_MINOR_RADIUS = 0.03f;
const float SCALE_SHAFT_LENGTH = 1.1f;
const float SCALE_BOX_SIZE = 0.16f;
} // namespace

bool Gizmo::on_create(Params params) {
    if(!StageNode::on_create(params)) {
        return false;
    }

    set_editor_only(true);

    build_translate_handles();
    build_rotate_handles();
    build_scale_handles();

    set_mode(GIZMO_MODE_TRANSLATE);
    reposition_handles();

    if(scene && scene->window) {
        scene->window->register_event_listener(this);
    }

    return true;
}

void Gizmo::on_transformation_changed() {
    StageNode::on_transformation_changed();
    reposition_handles();
}

Gizmo::~Gizmo() {
    if(scene && scene->window) {
        scene->window->unregister_event_listener(this);
    }
}

Vec3 Gizmo::axis_direction(Axis axis) const {
    switch(axis) {
        case Axis::X:
            return Vec3(1, 0, 0);
        case Axis::Y:
            return Vec3(0, 1, 0);
        case Axis::Z:
            return Vec3(0, 0, 1);
    }
    return Vec3(0, 1, 0);
}

Quaternion Gizmo::axis_orientation(Axis axis) const {
    // The procedural cylinder()/cone() generators build geometry along
    // local +Y, so only X and Z handles need rotating onto their axis.
    switch(axis) {
        case Axis::X:
            return Quaternion(Vec3(0, 0, 1), Degrees(-90));
        case Axis::Y:
            return Quaternion();
        case Axis::Z:
            return Quaternion(Vec3(1, 0, 0), Degrees(90));
    }
    return Quaternion();
}

Color Gizmo::axis_color(Axis axis) const {
    switch(axis) {
        case Axis::X:
            return Color(0.85f, 0.2f, 0.2f, 1.0f);
        case Axis::Y:
            return Color(0.2f, 0.85f, 0.2f, 1.0f);
        case Axis::Z:
            return Color(0.2f, 0.4f, 0.9f, 1.0f);
    }
    return Color::white();
}

MaterialPtr Gizmo::axis_material(const Color& color) {
    auto mat = scene->assets->clone_default_material();
    mat->set_name("GizmoAxis");
    mat->set_lighting_enabled(false);
    mat->set_textures_enabled(0);
    mat->set_base_color(color);
    return mat;
}

void Gizmo::recolor_mesh(const MeshPtr& mesh, const Color& color) {
    mesh->vertex_data->move_to_start();
    for(uint32_t i = 0; i < mesh->vertex_data->count(); ++i) {
        mesh->vertex_data->color(color);
        mesh->vertex_data->move_next();
    }
    mesh->vertex_data->done();
}

void Gizmo::disable_shadows(Actor* actor) {
    // A gizmo handle is an editor/tooling overlay, not scene content - it
    // shouldn't cast a shadow onto (or receive one from) anything else.
    actor->set_shadow_cast(SHADOW_CAST_NEVER);
    actor->set_shadow_receive(SHADOW_RECEIVE_NEVER);
}

void Gizmo::build_translate_handles() {
    for(auto axis: {Axis::X, Axis::Y, Axis::Z}) {
        auto orientation = axis_orientation(axis);
        auto mat = axis_material(axis_color(axis));

        auto shaft_mesh =
            scene->assets->create_mesh(VertexSpecification::DEFAULT);
        shaft_mesh->create_submesh_as_cylinder("shaft", mat, SHAFT_DIAMETER,
                                               SHAFT_LENGTH, 12, 1);
        recolor_mesh(shaft_mesh, axis_color(axis));

        auto shaft = create_child<Actor>(shaft_mesh);
        shaft->set_editor_only(true);
        disable_shadows(shaft);
        translate_handles_.push_back(shaft);
        handle_infos_.push_back(
            {shaft, axis, SHAFT_LENGTH * 0.5f, orientation});

        auto tip_mesh =
            scene->assets->create_mesh(VertexSpecification::DEFAULT);
        tip_mesh->create_submesh_as_cone("tip", mat, TIP_DIAMETER, TIP_LENGTH);
        recolor_mesh(tip_mesh, axis_color(axis));

        auto tip = create_child<Actor>(tip_mesh);
        tip->set_editor_only(true);
        disable_shadows(tip);
        translate_handles_.push_back(tip);
        handle_infos_.push_back(
            {tip, axis, SHAFT_LENGTH + TIP_LENGTH * 0.5f, orientation});
    }
}

void Gizmo::build_rotate_handles() {
    for(auto axis: {Axis::X, Axis::Y, Axis::Z}) {
        auto orientation = axis_orientation(axis);
        auto mat = axis_material(axis_color(axis));

        auto ring_mesh =
            scene->assets->create_mesh(VertexSpecification::DEFAULT);
        ring_mesh->create_submesh_as_torus("ring", mat, RING_MAJOR_RADIUS,
                                           RING_MINOR_RADIUS);
        recolor_mesh(ring_mesh, axis_color(axis));

        auto ring = create_child<Actor>(ring_mesh);
        ring->set_editor_only(true);
        disable_shadows(ring);
        rotate_handles_.push_back(ring);
        handle_infos_.push_back({ring, axis, 0.0f, orientation});
    }
}

void Gizmo::build_scale_handles() {
    for(auto axis: {Axis::X, Axis::Y, Axis::Z}) {
        auto orientation = axis_orientation(axis);
        auto mat = axis_material(axis_color(axis));

        auto shaft_mesh =
            scene->assets->create_mesh(VertexSpecification::DEFAULT);
        shaft_mesh->create_submesh_as_cylinder(
            "shaft", mat, SHAFT_DIAMETER, SCALE_SHAFT_LENGTH, 12, 1);
        recolor_mesh(shaft_mesh, axis_color(axis));

        auto shaft = create_child<Actor>(shaft_mesh);
        shaft->set_editor_only(true);
        disable_shadows(shaft);
        scale_handles_.push_back(shaft);
        handle_infos_.push_back(
            {shaft, axis, SCALE_SHAFT_LENGTH * 0.5f, orientation});

        auto box_mesh =
            scene->assets->create_mesh(VertexSpecification::DEFAULT);
        box_mesh->create_submesh_as_cube("box", mat, SCALE_BOX_SIZE);
        recolor_mesh(box_mesh, axis_color(axis));

        auto box = create_child<Actor>(box_mesh);
        box->set_editor_only(true);
        disable_shadows(box);
        scale_handles_.push_back(box);
        handle_infos_.push_back(
            {box, axis, SCALE_SHAFT_LENGTH + SCALE_BOX_SIZE * 0.5f,
             orientation});
    }
}

void Gizmo::set_mode(GizmoMode mode) {
    mode_ = mode;

    for(auto n: translate_handles_) {
        n->set_visible(mode_ == GIZMO_MODE_TRANSLATE);
    }
    for(auto n: rotate_handles_) {
        n->set_visible(mode_ == GIZMO_MODE_ROTATE);
    }
    for(auto n: scale_handles_) {
        n->set_visible(mode_ == GIZMO_MODE_SCALE);
    }
}

Vec3 Gizmo::world_origin() const {
    return transform->position();
}

void Gizmo::reposition_handles() {
    Vec3 origin = world_origin();
    for(auto& info: handle_infos_) {
        info.node->transform->set_position(
            origin + axis_direction(info.axis) * info.offset);
        info.node->transform->set_orientation(info.orientation);
    }
}

Plane Gizmo::drag_plane(Axis axis, Camera* camera) const {
    Vec3 origin = world_origin();
    Vec3 axis_dir = axis_direction(axis);

    if(mode_ == GIZMO_MODE_ROTATE) {
        // The ring handle lies in the plane perpendicular to its axis.
        return Plane(axis_dir, origin);
    }

    // Translate/scale: use the plane that contains the axis line and
    // faces the camera as directly as possible - this is the standard
    // trick (used by Blender/Unity) for turning a 1D "drag along this
    // axis" gesture into a well-conditioned ray/plane intersection,
    // rather than trying to project the mouse ray onto the axis line
    // directly (which is ill-conditioned when the axis points toward the
    // camera).
    Vec3 to_camera = camera->transform->position() - origin;
    if(to_camera.length() < 1e-5f) {
        to_camera = Vec3(0, 0, 1);
    }
    to_camera = to_camera.normalized();

    Vec3 normal = axis_dir.cross(to_camera.cross(axis_dir));
    if(normal.length() < 1e-5f) {
        // Axis is pointing directly at/away from the camera - fall back
        // to an arbitrary plane containing the axis.
        Vec3 fallback = (std::abs(axis_dir.y) < 0.99f) ? Vec3(0, 1, 0)
                                                        : Vec3(1, 0, 0);
        normal = axis_dir.cross(fallback);
    }

    return Plane(normal.normalized(), origin);
}

smlt::optional<Vec3> Gizmo::hit_point_on_plane(
    const Plane& plane, Camera* camera, const RenderTarget& target,
    const Viewport& viewport, const Vec2& screen_point) const {

    auto near_point =
        camera->unproject_point(target, viewport, Vec3(screen_point.x, screen_point.y, 0.0f));
    auto far_point =
        camera->unproject_point(target, viewport, Vec3(screen_point.x, screen_point.y, 1.0f));

    if(!near_point || !far_point) {
        return smlt::optional<Vec3>();
    }

    Ray ray(near_point.value(), far_point.value() - near_point.value());

    Vec3 hit;
    if(!ray.intersects_plane(plane, &hit)) {
        return smlt::optional<Vec3>();
    }

    return smlt::optional<Vec3>(hit);
}

float Gizmo::point_segment_distance_2d(const Vec2& p, const Vec2& a,
                                       const Vec2& b) {
    Vec2 ab = b - a;
    float len2 = ab.dot(ab);
    if(len2 < 1e-8f) {
        return (p - a).length();
    }

    float t = (p - a).dot(ab) / len2;
    t = std::max(0.0f, std::min(1.0f, t));

    Vec2 closest = a + ab * t;
    return (p - closest).length();
}

smlt::optional<Gizmo::Axis> Gizmo::hit_test(Camera* camera,
                                            const RenderTarget& target,
                                            const Viewport& viewport,
                                            const Vec2& screen_point) const {
    if(!camera) {
        return smlt::optional<Axis>();
    }

    Vec3 origin = world_origin();
    auto origin_screen = camera->project_point(target, viewport, origin);
    if(!origin_screen) {
        return smlt::optional<Axis>();
    }
    Vec2 origin_2d(origin_screen->x, origin_screen->y);

    const float HIT_RADIUS_PX = 14.0f;
    bool found = false;
    Axis best_axis = Axis::X;
    float best_dist = HIT_RADIUS_PX;

    auto consider_point = [&](Axis axis, const Vec2& a, const Vec2& b) {
        float dist = point_segment_distance_2d(screen_point, a, b);
        if(dist < best_dist) {
            best_dist = dist;
            best_axis = axis;
            found = true;
        }
    };

    if(mode_ == GIZMO_MODE_TRANSLATE || mode_ == GIZMO_MODE_SCALE) {
        float length = (mode_ == GIZMO_MODE_TRANSLATE)
                          ? (SHAFT_LENGTH + TIP_LENGTH)
                          : (SCALE_SHAFT_LENGTH + SCALE_BOX_SIZE);

        for(auto axis: {Axis::X, Axis::Y, Axis::Z}) {
            Vec3 dir = axis_direction(axis);
            auto end_screen =
                camera->project_point(target, viewport, origin + dir * length);
            if(!end_screen) {
                continue;
            }
            consider_point(axis, origin_2d, Vec2(end_screen->x, end_screen->y));
        }
    } else {
        // Rotate: sample points around each ring and test distance to the
        // resulting screen-space polyline.
        const int RING_SAMPLES = 24;
        const float TWO_PI = 6.28318530718f;

        for(auto axis: {Axis::X, Axis::Y, Axis::Z}) {
            Vec3 n = axis_direction(axis);
            Vec3 arbitrary = (std::abs(n.y) < 0.99f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
            Vec3 u = n.cross(arbitrary).normalized();
            Vec3 v = n.cross(u).normalized();

            smlt::optional<Vec2> prev;
            for(int i = 0; i <= RING_SAMPLES; ++i) {
                float theta = (float(i) / float(RING_SAMPLES)) * TWO_PI;
                Vec3 p = origin + (u * std::cos(theta) + v * std::sin(theta)) *
                                     RING_MAJOR_RADIUS;

                auto p_screen = camera->project_point(target, viewport, p);
                if(!p_screen) {
                    prev = smlt::optional<Vec2>();
                    continue;
                }

                Vec2 p2d(p_screen->x, p_screen->y);
                if(prev) {
                    consider_point(axis, prev.value(), p2d);
                }
                prev = smlt::optional<Vec2>(p2d);
            }
        }
    }

    if(found) {
        return smlt::optional<Axis>(best_axis);
    }
    return smlt::optional<Axis>();
}

bool Gizmo::begin_drag(Camera* camera, const RenderTarget& target,
                       const Viewport& viewport, const Vec2& screen_point) {
    auto axis = hit_test(camera, target, viewport, screen_point);
    if(!axis) {
        return false;
    }

    // Fix the axis direction and drag plane for the whole gesture now,
    // before anything moves - see the comment on drag_plane_fixed_.
    drag_axis_ = axis.value();
    drag_axis_dir_ = axis_direction(drag_axis_);
    drag_plane_fixed_ = drag_plane(drag_axis_, camera);

    auto hit =
        hit_point_on_plane(drag_plane_fixed_, camera, target, viewport,
                           screen_point);
    if(!hit) {
        return false;
    }

    dragging_ = true;
    drag_start_hit_point_ = hit.value();
    drag_start_position_ = transform->position();
    drag_start_orientation_ = transform->orientation();
    drag_start_scale_ = transform->scale();
    return true;
}

void Gizmo::update_drag(Camera* camera, const RenderTarget& target,
                        const Viewport& viewport, const Vec2& screen_point) {
    if(!dragging_) {
        return;
    }

    auto hit = hit_point_on_plane(drag_plane_fixed_, camera, target, viewport,
                                  screen_point);
    if(!hit) {
        return;
    }

    Vec3 axis_dir = drag_axis_dir_;
    Vec3 delta = hit.value() - drag_start_hit_point_;

    switch(mode_) {
        case GIZMO_MODE_TRANSLATE: {
            float d = delta.dot(axis_dir);
            transform->set_position(drag_start_position_ + axis_dir * d);
            break;
        }
        case GIZMO_MODE_SCALE: {
            float d = delta.dot(axis_dir);
            float factor = std::max(0.01f, 1.0f + d / SCALE_SHAFT_LENGTH);

            Vec3 new_scale = drag_start_scale_;
            switch(drag_axis_) {
                case Axis::X:
                    new_scale.x = drag_start_scale_.x * factor;
                    break;
                case Axis::Y:
                    new_scale.y = drag_start_scale_.y * factor;
                    break;
                case Axis::Z:
                    new_scale.z = drag_start_scale_.z * factor;
                    break;
            }
            transform->set_scale(new_scale);
            break;
        }
        case GIZMO_MODE_ROTATE: {
            Vec3 origin = drag_start_position_;
            Vec3 v0 = drag_start_hit_point_ - origin;
            Vec3 v1 = hit.value() - origin;

            // Both points should already lie (almost) on the plane
            // perpendicular to the axis, but strip any residual axis
            // component so the angle calculation is well-defined.
            v0 = v0 - axis_dir * v0.dot(axis_dir);
            v1 = v1 - axis_dir * v1.dot(axis_dir);

            if(v0.length() < 1e-5f || v1.length() < 1e-5f) {
                break;
            }
            v0 = v0.normalized();
            v1 = v1.normalized();

            float cos_angle = std::max(-1.0f, std::min(1.0f, v0.dot(v1)));
            float angle_rad = std::acos(cos_angle);

            Vec3 cross = v0.cross(v1);
            float sign = (cross.dot(axis_dir) < 0.0f) ? -1.0f : 1.0f;

            Quaternion delta_rot(axis_dir, Radians(angle_rad * sign));
            transform->set_orientation(
                (delta_rot * drag_start_orientation_).normalized());
            break;
        }
    }
}

void Gizmo::end_drag() {
    dragging_ = false;
}

void Gizmo::on_mouse_down(const MouseEvent& evt) {
    if(evt.button != 0 || !camera_ || !scene || !scene->window) {
        return;
    }

    Vec2 screen_point(float(evt.x), float(evt.y));
    begin_drag(camera_, *scene->window, Viewport(), screen_point);
}

void Gizmo::on_mouse_up(const MouseEvent& evt) {
    if(evt.button != 0) {
        return;
    }
    end_drag();
}

void Gizmo::on_mouse_move(const MouseEvent& evt) {
    if(!dragging_ || !camera_ || !scene || !scene->window) {
        return;
    }

    Vec2 screen_point(float(evt.x), float(evt.y));
    update_drag(camera_, *scene->window, Viewport(), screen_point);
}

} // namespace smlt
