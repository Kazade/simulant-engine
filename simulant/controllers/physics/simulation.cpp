#include "simulation.h"
#include "body.h"

#include "../../deps/bounce/bounce.h"

namespace smlt{
namespace controllers {

void to_b3vec3(const Vec3& rhs, b3Vec3& ret) {
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
}

void to_vec3(const b3Vec3& rhs, Vec3& ret) {
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
}

void to_mat3(const b3Mat33& rhs, Mat3& out) {
    Mat3 ret((float*)&rhs[0]);
    out = ret;
}

void to_quat(const b3Quat& rhs, Quaternion& out) {
    out = Quaternion(
        rhs.x,
        rhs.y,
        rhs.z,
        rhs.w
    );
}

RigidBodySimulation::RigidBodySimulation(TimeKeeper *time_keeper):
    time_keeper_(time_keeper) {

    scene_.reset(new b3World());
    scene_->SetGravity(b3Vec3(0, -9.81, 0));
}

void RigidBodySimulation::set_gravity(const Vec3& gravity) {
    b3Vec3 g;
    to_b3vec3(gravity, g);
    scene_->SetGravity(g);
}

bool RigidBodySimulation::init() {

    return true;
}

void RigidBodySimulation::cleanup() {

}



void RigidBodySimulation::fixed_update(float step) {
    uint32_t velocity_iterations = 8;
    uint32_t position_iterations = 2;

    signal_simulation_pre_step_();
    scene_->Step(step, velocity_iterations, position_iterations);
}

std::pair<Vec3, bool> RigidBodySimulation::intersect_ray(const Vec3& start, const Vec3& direction, float* distance, Vec3* normal) {
    b3RayCastSingleOutput result;
    b3Vec3 s, d;

    to_b3vec3(start, s);
    to_b3vec3(start + direction, d);

    bool hit = scene_->RayCastSingle(&result, s, d);

    float closest = std::numeric_limits<float>::max();
    Vec3 impact_point, closest_normal;

    if(hit) {
        to_vec3(result.point, impact_point);
        closest = (impact_point - start).length();
        to_vec3(result.normal, closest_normal);
    }

    // Now, check all the raycast only colliders
    for(auto& p: raycast_colliders_) {
        float hit_distance;
        Vec3 n;
        auto ret = p.second.intersect_ray(start, direction, &hit_distance, &n);
        if(ret.second) {
            // We hit something
            if(hit_distance < closest) {
                closest = hit_distance;
                impact_point = ret.first;
                hit = true;
                closest_normal = n;
            }
        }
    }

    if(distance) {
        *distance = closest;
    }

    if(normal) {
        *normal = closest_normal;
    }

    return std::make_pair(impact_point, hit);
}

b3Body *RigidBodySimulation::acquire_body(impl::Body *body) {
    b3BodyDef def;

    bool is_dynamic = body->is_dynamic();
    def.type = (is_dynamic) ? b3BodyType::e_dynamicBody : b3BodyType::e_staticBody;
    def.gravityScale = (is_dynamic) ? 1.0 : 0.0;
    def.userData = this;

    bodies_[body] = scene_->CreateBody(def);
    return bodies_[body];
}

void RigidBodySimulation::release_body(impl::Body *body) {
    scene_->DestroyBody(bodies_.at(body));
}

std::pair<Vec3, Quaternion> RigidBodySimulation::body_transform(const impl::Body *body) {
    b3Body* b = bodies_.at(body);

    auto tx = b->GetTransform();

    Vec3 p;
    to_vec3(tx.position, p);

    Mat3 r;
    to_mat3(tx.rotation, r);

    return std::make_pair(
        p,
        Quaternion(r)
    );
}

void RigidBodySimulation::set_body_transform(impl::Body* body, const Vec3& position, const Quaternion& rotation) {
    b3Body* b = bodies_.at(body);

    auto axis_angle = rotation.to_axis_angle();

    b3Vec3 p, a;
    to_b3vec3(position, p);
    to_b3vec3(axis_angle.axis, a);
    b->SetTransform(p, a, axis_angle.angle.value);
}


}
}
