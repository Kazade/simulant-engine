
#include "raycast_vehicle.h"

#include "../object.h"

namespace kglt {
namespace controllers {

RaycastVehicle::RaycastVehicle(kglt::Controllable* object, RigidBodySimulation::ptr simulation, float wheel_height):
    RigidBody(object, simulation, COLLIDER_TYPE_BOX),
    wheel_height_(wheel_height) {


    BoundableEntity* entity = dynamic_cast<BoundableEntity*>(object_);
    AABB aabb = entity->aabb();

    float offset = 0.001;

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

void RaycastVehicle::do_fixed_update(double dt) {
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
        auto ret = simulation_->intersect_ray(ray.start, ray.dir, &intersection.dist, &intersection.normal);

        // If we intersected
        if(ret.second) {
            // Store the intersection information
            intersection.point = ret.first;
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
        Vec3 Cf;
        kmVec3ProjectOnToVec3(&Cv, &up, &Cf); // Project Cv onto Up

        // Calculate a value between 0.0 and 1.0 that shows how compressed the suspension is
        float compression_ratio = 1.0 - (intersection.dist / intersection.ray_dir.length());

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
        if(fabs(drive_force_) > kmEpsilon) {
            average_surface_normal_.normalize();
            Vec3 dir = forward();
            add_force(dir * drive_force_);
        }
        if(fabs(turn_force_) > kmEpsilon) {
            Vec3 torque = Vec3(0, 1, 0);
            add_torque(torque * turn_force_);
        }
    }
}

}
}
