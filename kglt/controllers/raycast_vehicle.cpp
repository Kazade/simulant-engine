
#include "raycast_vehicle.h"

#include "../object.h"

namespace kglt {
namespace controllers {

RaycastVehicle::RaycastVehicle(kglt::Controllable* object, RigidBodySimulation::ptr simulation, float wheel_height):
    RigidBody(object, simulation, COLLIDER_TYPE_BOX),
    wheel_height_(wheel_height) {


    BoundableEntity* entity = dynamic_cast<BoundableEntity*>(object_);
    AABB aabb = entity->aabb();

    float offset = 0.1;

    wheel_position_local_[0] = Vec3(aabb.min.x, aabb.min.y - offset, aabb.min.z);
    wheel_position_local_[1] = Vec3(aabb.max.x, aabb.min.y - offset, aabb.min.z);
    wheel_position_local_[2] = Vec3(aabb.min.x, aabb.min.y - offset, aabb.max.z);
    wheel_position_local_[3] = Vec3(aabb.max.x, aabb.min.y - offset, aabb.max.z);

    ground_detector_local_ = Vec3(0, aabb.min.y - offset, 0);
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
    ground_detector_.dir = (DOWN * rot).normalized() * (wheel_height_ / 2.0);
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

void RaycastVehicle::do_update(double dt) {
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
    };

    std::map<float, std::vector<Intersection>> intersections_by_distance;

    for(auto& ray: rays) {
        Intersection intersection;
        auto ret = simulation_->intersect_ray(ray.start, ray.dir, &intersection.dist, &intersection.normal);
        if(ret.second) {
            intersection.point = ret.first;
            intersection.penetration = Vec3(ray.dir).length() - intersection.dist;
            intersections_by_distance[intersection.dist].push_back(intersection);
        }
    }

    if(!intersections_by_distance.empty()) {
        auto intersections_to_process = *(intersections_by_distance.rbegin()); // Get the max key (rbegin)

        // First move the vehicle backwards along the velocity until we're no longer colliding
        auto reverse_normalized_vel = -linear_velocity().normalized();
        auto dist = intersections_to_process.second[0].penetration;
        auto to_move = reverse_normalized_vel * dist;

        // Move out a bit
        Vec3 pos = position();
        this->move_to(pos + to_move);

        // Now apply impulses for all colliders at this (max) distance
        for(auto& intersection: intersections_to_process.second) {
            Vec3 linear_vel_at = -linear_velocity_at(intersection.point);
            Vec3 proj;
            kmVec3ProjectOnToVec3(&linear_vel_at, &intersection.normal, &proj);

            float vel = proj.length();
            add_impulse(intersection.normal * vel * mass());
        }
    }
}

}
}
