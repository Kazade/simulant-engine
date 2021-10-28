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

#include "../../interfaces/boundable.h"
#include "raycast_vehicle.h"
#include "simulation.h"

namespace smlt {
namespace behaviours {

static const float FLOAT_EPSILON = std::numeric_limits<float>::epsilon();

RaycastVehicle::RaycastVehicle(RigidBodySimulation* simulation, float wheel_height):
    RigidBody(simulation),
    wheel_height_(wheel_height) {


    BoundableEntity* entity = dynamic_cast<BoundableEntity*>(stage_node.get());
    AABB aabb = entity->aabb();

    float offset = 0.001f;

    auto min = aabb.min();
    auto max = aabb.max();

    wheel_position_local_[0] = Vec3(min.x, min.y - offset, min.z);
    wheel_position_local_[1] = Vec3(max.x, min.y - offset, min.z);
    wheel_position_local_[2] = Vec3(min.x, min.y - offset, max.z);
    wheel_position_local_[3] = Vec3(max.x, min.y - offset, max.z);

    ground_detector_local_ = Vec3(0, min.y - offset, 0);
}

void RaycastVehicle::recalculate_rays() {
    const Vec3 DOWN(0, -1, 0);

    auto rot = rotation();
    auto pos = position();

    for(uint8_t i = 0; i < 4; ++i) {
        wheels_[i].start = wheel_position_local_[i] * rot;
        wheels_[i].start = Vec3(wheels_[i].start) + pos;

        wheels_[i].dir = (DOWN * rot).normalized() * wheel_height_;
    }

    ground_detector_.start = ground_detector_local_ * rot;
    ground_detector_.start = Vec3(ground_detector_.start) + pos;
    ground_detector_.dir = (DOWN * rot).normalized() * (wheel_height_ / 2.0f);
}

bool RaycastVehicle::init() {
    if(RigidBody::init()) {
        // Initialize ray positions
        recalculate_rays();
        return true;
    } else {
        return false;
    }
}

void RaycastVehicle::fixed_update(float dt) {
    _S_UNUSED(dt);

    RigidBodySimulation* sim = simulation;
    if(!sim) {
        return;
    }

    recalculate_rays();

    // If a roll has been detected, then fire a signal and do nothing else
    if(roll_detected() && !is_rolling_) {
        signal_roll_detected_(this);
        is_rolling_ = true;
        return;
    } else if(is_rolling_) {
        is_rolling_ = false;
    }

    std::vector<Ray> rays = { wheels_[0], wheels_[1], wheels_[2], wheels_[3], ground_detector_ };

    struct Intersection {
        float dist;
        Vec3 normal;
        Vec3 point;
        float penetration;
        Vec3 ray_start;
        Vec3 ray_dir;
    };

    std::vector<Intersection> intersections;

    for(auto& ray: rays) {
        Intersection intersection;
        auto ret = sim->ray_cast(
            ray.start, ray.dir.normalized(), ray.dir.length()
        );

        // If we intersected
        if(ret) {
            // Store the intersection information
            intersection.point = ret.value().impact_point;
            intersection.dist = ret.value().distance;
            intersection.normal = ret.value().normal;
            intersection.penetration = Vec3(ray.dir).length() - intersection.dist;
            intersection.ray_dir = Vec3(ray.dir);
            intersection.ray_start = Vec3(ray.start);
            intersections.push_back(intersection);
        }
    }

    const float SUSPENSION_STIFFNESS = 20;

    Vec3 average_surface_normal_;
    is_grounded_ = !intersections.empty();

    for(auto& intersection: intersections) {
        Vec3 up = -intersection.ray_dir.normalized(); //Vehicle up is always the reverse of the ray direction
        Vec3 Cv = linear_velocity_at(intersection.ray_start); // Get the velocity at the point the ray starts
        Vec3 Cf = up.project_onto_vec3(Cv);

        // Calculate a value between 0.0 and 1.0 that shows how compressed the suspension is
        float compression_ratio = 1.0f - (intersection.dist / intersection.ray_dir.length());

        auto Nf = up * compression_ratio * SUSPENSION_STIFFNESS;
        auto Df = Nf - Cf;

        add_force_at_position(Df, intersection.ray_start);

        average_surface_normal_ += intersection.normal;

        std::cout << "Cr: " << compression_ratio << std::endl;
        std::cout << "Nf: " << Nf.x << ", " << Nf.y << ", " << Nf.z << std::endl;
        std::cout << "Cf: " << Cf.x << ", " << Cf.y << ", " << Cf.z << std::endl;
        std::cout << "Df: " << Df.x << ", " << Df.y << ", " << Df.z << std::endl;
        std::cout << "===============================================" << std::endl;
    }

    if(is_grounded_) {
        average_surface_normal_.normalize();
        Plane ground_plane(average_surface_normal_, 0);
        Vec3 proj_forward = ground_plane.project(forward());

        std::cout << "Sn: " << average_surface_normal_.x << ", " << average_surface_normal_.y << ", " << average_surface_normal_.z << std::endl;
        std::cout << "Fo: " << forward().x << ", " << forward().y << ", " << forward().z << std::endl;
        std::cout << "Pf: " << proj_forward.x << ", " << proj_forward.y << ", " << proj_forward.z << std::endl;

        if(fabsf(drive_force_) > FLOAT_EPSILON) {
            average_surface_normal_.normalize();
            add_force(proj_forward * drive_force_);
        }
        if(fabsf(turn_force_) > FLOAT_EPSILON) {
            Vec3 torque = Vec3(0, 1, 0);
            add_torque(torque * turn_force_);
        }

        // Apply side force
        Vec3 linear_vel = linear_velocity();
        Vec3 proj = linear_vel.project_onto_vec3(proj_forward);

        Vec3 floor_vel = ground_plane.project(linear_vel);
        Vec3 Sf = proj - floor_vel;
        std::cout << "Sf: " << Sf.x << ", " << Sf.y << ", " << Sf.z << std::endl;
        add_force(Sf * 10.0f);

        // Apply some friction
        Vec3 friction_force = -proj * mass() * 0.01;
        add_force(friction_force);


    }
}

}
}
