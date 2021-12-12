#include "bounce/bounce.h"

#include "simulation.h"
#include "body.h"

#include "../../nodes/stage_node.h"
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
        b3Fixture* fixtureA = contact->GetFixtureA();
        b3Fixture* fixtureB = contact->GetFixtureB();

        Body* bodyA = (Body*) fixtureA->GetUserData();
        Body* bodyB = (Body*) fixtureB->GetUserData();

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

        b3Fixture* fixtureA = contact->GetFixtureA();
        b3Fixture* fixtureB = contact->GetFixtureB();

        Body* bodyA = (Body*) fixtureA->GetUserData();
        Body* bodyB = (Body*) fixtureB->GetUserData();

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

            if(contact->GetFixtureA()->GetBody() == body || contact->GetFixtureB()->GetBody() == body) {
                ret.push_back(contact);
            }
        }

        return ret;
    }

private:
    std::pair<Collision, Collision> build_collision_pair(b3Contact* contact) {
        b3Fixture* fixtureA = contact->GetFixtureA();
        b3Fixture* fixtureB = contact->GetFixtureB();

        Body* bodyA = (Body*) fixtureA->GetUserData();
        Body* bodyB = (Body*) fixtureB->GetUserData();

        Collision collA, collB;

        collA.this_body = bodyA;
        collA.this_collider_name = bodyA->collider_details_.at(fixtureA).name;
        collA.this_stage_node = bodyA->stage_node.get();

        collA.other_body = bodyB;
        collA.other_collider_name = bodyB->collider_details_.at(fixtureB).name;
        collA.other_stage_node = bodyB->stage_node.get();

        collB.other_body = bodyA;
        collB.other_collider_name = bodyA->collider_details_.at(fixtureA).name;
        collB.other_stage_node = bodyA->stage_node.get();

        collB.this_body = bodyB;
        collB.this_collider_name = bodyB->collider_details_.at(fixtureB).name;
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

smlt::optional<RayCastResult> RigidBodySimulation::ray_cast(const Vec3& start, const Vec3& direction, float max_distance) {
    b3RayCastSingleOutput result;
    b3Vec3 s, d;

    to_b3vec3(start, s);
    to_b3vec3(start + (direction * max_distance), d);

    struct AlwaysCast : public b3RayCastFilter {
        bool ShouldRayCast(b3Fixture*) override {
            return true;
        }
    };

    AlwaysCast filter;

    bool hit = scene_->RayCastSingle(&result, &filter, s, d);

    float closest = std::numeric_limits<float>::max();

    if(hit) {
        RayCastResult ret;
        ret.other_body = get_associated_body(result.fixture->GetBody());
        to_vec3(result.point, ret.impact_point);
        closest = (ret.impact_point - start).length();
        to_vec3(result.normal, ret.normal);
        ret.distance = closest;
        return smlt::optional<RayCastResult>(ret);
    }

    return smlt::optional<RayCastResult>();
}

b3Body *RigidBodySimulation::acquire_body(impl::Body *body) {
    b3BodyDef def;

    bool is_dynamic = body->is_dynamic();
    bool is_kinematic = body->is_kinematic();

    if(is_kinematic) {
        def.type = b3BodyType::e_kinematicBody;
    } else if(is_dynamic) {
        def.type = b3BodyType::e_dynamicBody;
    } else {
        def.type = b3BodyType::e_staticBody;
    }

    /* Kinematic bodies are dynamic bodies */
    b3Vec3 v;
    v.x = (is_dynamic) ? 1.0f : 0.0f;
    v.y = (is_dynamic) ? 1.0f : 0.0f;
    v.z = (is_dynamic) ? 1.0f : 0.0f;

    def.gravityScale = v;
    def.userData = body;

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
    auto it = bodies_.find(body);
    if(it != bodies_.end()) {
        auto bbody = (*it).second;
        scene_->DestroyBody(bbody);
        bodies_.erase(it);
    }
}

impl::Body *RigidBodySimulation::get_associated_body(b3Body *b) {
    return (impl::Body*) b->GetUserData();
}

std::pair<Vec3, Quaternion> RigidBodySimulation::body_transform(const impl::Body *body) {
    b3Body* b = body->body_;

    auto position = b->GetWorldCenter();
    auto rotation = b->GetOrientation();

    Vec3 p;
    to_vec3(position, p);

    Quaternion r;
    to_quat(rotation, r);

    return std::make_pair(p, r);
}

void RigidBodySimulation::set_body_transform(impl::Body* body, const Vec3& position, const Quaternion& rotation) {
    b3Body* b = body->body_;

    b3Vec3 p;
    to_b3vec3(position, p);

    b3Quat rot;
    to_b3quat(rotation, rot);

    b->SetTransform(p, rot);
}


}
}
