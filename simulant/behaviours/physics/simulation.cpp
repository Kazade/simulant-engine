#include "simulation.h"
#include "body.h"
#include "../../nodes/stage_node.h"
#include "../../deps/bounce/bounce.h"
#include "../../macros.h"

/* Need for bounce */
void b3BeginProfileScope(const char* name) {
    _S_UNUSED(name);
}

void b3EndProfileScope() {

}

namespace smlt{
namespace behaviours {

void to_b3vec3(const Vec3& rhs, b3Vec3& ret) {
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
}

void to_b3quat(const Quaternion& q, b3Quat& ret) {
    ret.v.x = q.x;
    ret.v.y = q.y;
    ret.v.z = q.z;
    ret.s = q.w;
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
        rhs.v.x,
        rhs.v.y,
        rhs.v.z,
        rhs.s
    );
}

namespace impl {

class ContactListener : public b3ContactListener {
public:
    ContactListener(RigidBodySimulation* simulation):
        simulation_(simulation) {

    }

    virtual ~ContactListener() {}

    void BeginContact(b3Contact* contact) {
        b3Shape* shapeA = contact->GetShapeA();
        b3Shape* shapeB = contact->GetShapeB();

        Body* bodyA = (Body*) shapeA->GetUserData();
        Body* bodyB = (Body*) shapeB->GetUserData();

        if(simulation_->body_exists(bodyA) && simulation_->body_exists(bodyB)) {
            auto coll_pair = build_collision_pair(contact);
            auto& collA = coll_pair.first;
            auto& collB = coll_pair.second;

            bodyA->contact_started(collA);
            bodyB->contact_started(collB);

            // FIXME: Populate contact points

            active_contacts_.insert(contact);
        }
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

        if(simulation_->body_exists(bodyA) && simulation_->body_exists(bodyB)) {
            auto coll_pair = build_collision_pair(contact);
            auto& collA = coll_pair.first;
            auto& collB = coll_pair.second;

            bodyA->contact_finished(collA);
            bodyB->contact_finished(collB);

            active_contacts_.erase(contact);
        } else {
            // If they don't exist but we still find the contact, then that's a problem!
            assert(!active_contacts_.count(contact));
        }
    }

    void PreSolve(b3Contact* contact) {
        _S_UNUSED(contact);
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
    std::pair<Collision, Collision> build_collision_pair(b3Contact* contact) {
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

        collB.other_body = bodyA;
        collB.other_collider_name = bodyA->collider_details_.at(shapeA).name;
        collB.other_stage_node = bodyA->stage_node.get();

        collB.this_body = bodyB;
        collB.this_collider_name = bodyB->collider_details_.at(shapeB).name;
        collB.this_stage_node = bodyB->stage_node.get();

        return std::make_pair(collA, collB);
    }

    std::set<b3Contact*> active_contacts_;
    RigidBodySimulation* simulation_;
};

}


RigidBodySimulation::RigidBodySimulation(TimeKeeper *time_keeper):
    time_keeper_(time_keeper) {

    contact_listener_ = std::make_shared<impl::ContactListener>(this);

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

void RigidBodySimulation::clean_up() {
    // Disconnect the contact listener
    scene_->SetContactListener(nullptr);
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

    static struct RayCastFilter : public b3RayCastFilter {
        virtual bool ShouldRayCast(b3Shape* shape) override {
            _S_UNUSED(shape);
            return true;
        }
    } filter;

    bool hit = scene_->RayCastSingle(&result, &filter, s, d);

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

    float gs = (is_dynamic) ? 1.0 : 0.0;

    def.gravityScale.x = gs;
    def.gravityScale.y = gs;
    def.gravityScale.z = gs;

    def.userData = this;

    // If the body is attached to a stage node then set up the initial rotation
    // and position from that.

    if(body->stage_node) {
        to_b3vec3(body->stage_node->absolute_position(), def.position);
        to_b3quat(body->stage_node->absolute_rotation(), def.orientation);
    }

    bodies_[body] = scene_->CreateBody(def);
    return bodies_[body];
}

void RigidBodySimulation::release_body(impl::Body *body) {
    auto bbody = bodies_.at(body);
    scene_->DestroyBody(bbody);
    bodies_.erase(body);
}

std::pair<Vec3, Quaternion> RigidBodySimulation::body_transform(const impl::Body *body) {
    b3Body* b = bodies_.at(body);

    auto position = b->GetWorldCenter();
    auto rotation = b->GetOrientation();

    Vec3 p;
    to_vec3(position, p);

    Quaternion r;
    to_quat(rotation, r);

    return std::make_pair(p, r);
}

void RigidBodySimulation::set_body_transform(impl::Body* body, const Vec3& position, const Quaternion& rotation) {
    b3Body* b = bodies_.at(body);

    auto axis_angle = rotation.to_axis_angle();

    b3Vec3 p, a;
    to_b3vec3(position, p);
    to_b3vec3(axis_angle.axis, a);

    b3Quat q;
    q.SetAxisAngle(a, axis_angle.angle.value);
    b->SetTransform(p, q);
}


}
}
