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

void to_b3quat(const Quaternion& q, b3Quat& ret) {
    ret.x = q.x;
    ret.y = q.y;
    ret.z = q.z;
    ret.w = q.w;
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

namespace impl {

class ContactListener : public b3ContactListener {
public:
    void BeginContact(b3Contact* contact) {
        b3Shape* shapeA = contact->GetShapeA();
        b3Shape* shapeB = contact->GetShapeB();

        Body* bodyA = (Body*) shapeA->GetUserData();
        Body* bodyB = (Body*) shapeB->GetUserData();

        Collision collA, collB;

        collA.this_body = bodyA;
        collA.this_collider_name = bodyA->collider_details_.at(shapeA).name;
        collA.this_stage_node = bodyA->stage_node.get();

        collA.other_body = bodyB;
        collA.other_collider_name = bodyB->collider_details_.at(shapeB).name;
        collA.other_stage_node = bodyB->stage_node.get();

        bodyA->contact_started(collA);

        collB.other_body = bodyA;
        collB.other_collider_name = bodyA->collider_details_.at(shapeA).name;
        collB.other_stage_node = bodyA->stage_node.get();

        collB.this_body = bodyB;
        collB.this_collider_name = bodyB->collider_details_.at(shapeB).name;
        collB.this_stage_node = bodyB->stage_node.get();

        bodyB->contact_started(collB);

        // FIXME: Populate contact points

        active_contacts_.insert(contact);
    }

    void EndContact(b3Contact* contact) {
        if(active_contacts_.count(contact) == 0) {
            // Already released
            return;
        }

        b3Shape* shapeA = contact->GetShapeA();
        b3Shape* shapeB = contact->GetShapeB();

        Body* bodyA = (Body*) shapeA->GetUserData();
        Body* bodyB = (Body*) shapeB->GetUserData();

        bodyA->contact_finished();
        bodyB->contact_finished();

        active_contacts_.erase(contact);
    }

    void PreSolve(b3Contact* contact) {

    }

    std::vector<b3Contact*> ActiveContactsForBody(b3Body* body) {
        std::vector<b3Contact*> ret;
        for(auto contact: active_contacts_) {

            if(contact->GetShapeA()->GetBody() == body || contact->GetShapeB()->GetBody() == body) {
                ret.push_back(contact);
            }
        }

        return ret;
    }

private:
    std::set<b3Contact*> active_contacts_;
};

}


RigidBodySimulation::RigidBodySimulation(TimeKeeper *time_keeper):
    time_keeper_(time_keeper) {

    contact_listener_ = std::make_shared<impl::ContactListener>();

    scene_.reset(new b3World());
    scene_->SetGravity(b3Vec3(0, -9.81, 0));
    scene_->SetContactListener(contact_listener_.get());
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

        if(distance) {
            *distance = closest;
        }

        if(normal) {
            *normal = closest_normal;
        }
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
    auto bbody = bodies_.at(body);
    scene_->DestroyBody(bbody);
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
