//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "rigid_body.h"
#include "../../nodes/actor.h"
#include "../../stage.h"
#include "../../deps/bounce/bounce.h"


// These are just to keep Bounce happy
bool b3PushProfileScope(char const*) { return false; }
void b3PopProfileScope() {}


namespace smlt {
namespace controllers {

b3Vec3 to_b3vec3(const Vec3& rhs) {
    b3Vec3 ret;
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
    return ret;
}

Vec3 to_vec3(const b3Vec3& rhs) {
    Vec3 ret;
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
    return ret;
}

Mat3 to_mat3(const b3Mat33& rhs) {
    Mat3 ret((float*)&rhs[0]);
    return ret;
}

Quaternion to_quat(const b3Quat& rhs) {
    return Quaternion(
        rhs.x,
        rhs.y,
        rhs.z,
        rhs.w
    );
}

std::pair<Vec3, Vec3> calculate_bounds(const std::vector<Vec3>& vertices) {
    float min = std::numeric_limits<float>::max(), max = std::numeric_limits<float>::lowest();

    for(auto& vertex: vertices) {
        if(vertex.x < min) min = vertex.x;
        if(vertex.y < min) min = vertex.y;
        if(vertex.z < min) min = vertex.z;
        if(vertex.x > max) max = vertex.x;
        if(vertex.y > max) max = vertex.y;
        if(vertex.z > max) max = vertex.z;
    }

    return std::make_pair(Vec3(min, min, min), Vec3(max, max, max));
}


RigidBodySimulation::RigidBodySimulation(TimeKeeper *time_keeper):
    time_keeper_(time_keeper) {

    scene_.reset(new b3World());
    scene_->SetGravity(b3Vec3(0, -9.81, 0));
}

void RigidBodySimulation::set_gravity(const Vec3& gravity) {
    scene_->SetGravity(to_b3vec3(gravity));
}

bool RigidBodySimulation::init() {

    return true;
}

void RigidBodySimulation::cleanup() {

}



void RigidBodySimulation::fixed_update(float step) {
    uint32_t velocity_iterations = 8;
    uint32_t position_iterations = 2;

    scene_->Step(step, velocity_iterations, position_iterations);
}

std::pair<Vec3, bool> RigidBodySimulation::intersect_ray(const Vec3& start, const Vec3& direction, float* distance, Vec3* normal) {
    b3RayCastSingleOutput result;
    bool hit = scene_->RayCastSingle(&result, to_b3vec3(start), to_b3vec3(start + direction));

    float closest = std::numeric_limits<float>::max();
    Vec3 impact_point, closest_normal;

    if(hit) {
        impact_point = to_vec3(result.point);
        closest = (impact_point - start).length();
        closest_normal = to_vec3(result.normal);
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

    return std::make_pair(
        to_vec3(tx.position),
        Quaternion(to_mat3(tx.rotation))
    );
}

void RigidBodySimulation::set_body_transform(impl::Body* body, const Vec3& position, const Quaternion& rotation) {
    b3Body* b = bodies_.at(body);

    auto axis_angle = rotation.to_axis_angle();

    b->SetTransform(to_b3vec3(position), to_b3vec3(axis_angle.axis), axis_angle.angle.value);
}

RigidBody::RigidBody(Controllable* object, RigidBodySimulation* simulation, ColliderType collider):
    Body(object, simulation, collider) {

}

RigidBody::~RigidBody() {

}

void RigidBody::add_force(const Vec3 &force) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b->ApplyForceToCenter(to_b3vec3(force), true);
}

void RigidBody::add_impulse(const Vec3& impulse) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b->ApplyLinearImpulse(to_b3vec3(impulse), b->GetPosition(), true);
}

void RigidBody::add_impulse_at_position(const Vec3& impulse, const Vec3& position) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b->ApplyLinearImpulse(to_b3vec3(impulse), to_b3vec3(position), true);
}

float RigidBody::mass() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return 0;
    }

    const b3Body* b = sim->bodies_.at(this);
    return b->GetMass();
}

Vec3 RigidBody::linear_velocity() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    const b3Body* b = sim->bodies_.at(this);
    return to_vec3(b->GetLinearVelocity());
}

Vec3 RigidBody::linear_velocity_at(const Vec3& position) const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    const b3Body* b = sim->bodies_.at(this);

    auto direction_to_point = to_b3vec3(position) - b->GetPosition();
    auto relative_torque = b3Cross(b->GetAngularVelocity(), direction_to_point);

    return to_vec3(b->GetLinearVelocity() + relative_torque);
}

Vec3 RigidBody::position() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Vec3();
    }

    return sim->body_transform(this).first;
}

Quaternion RigidBody::rotation() const {
    auto sim = simulation_.lock();
    if(!sim) {
        return Quaternion();
    }

    return sim->body_transform(this).second;
}

void RigidBody::add_force_at_position(const Vec3& force, const Vec3& position) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);

    b3Vec3 f, p;

    f.x = force.x;
    f.y = force.y;
    f.z = force.z;

    p.x = position.x;
    p.y = position.y;
    p.z = position.z;

    b->ApplyForce(to_b3vec3(force), to_b3vec3(position), true);
}

void RigidBody::add_torque(const Vec3& torque) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Body* b = sim->bodies_.at(this);
    b->ApplyTorque(to_b3vec3(torque), true);
}

StaticBody::StaticBody(Controllable* object, RigidBodySimulation* simulation, ColliderType collider):
    Body(object, simulation, collider) {

}

StaticBody::~StaticBody() {

}


namespace impl {

Body::Body(Controllable* object, RigidBodySimulation* simulation, ColliderType collider_type):
    Controller(),
    simulation_(simulation->shared_from_this()),
    collider_type_(collider_type) {

    object_ = dynamic_cast<StageNode*>(object);
    if(!object_) {
        throw std::runtime_error("Tried to attach a rigid body controller to something that isn't moveable");
    }
}

Body::~Body() {

}

bool Body::init() {
    auto sim = simulation_.lock();
    if(!sim) {
        return false;
    }

    body_ = sim->acquire_body(this);
    build_collider(collider_type_);

    return true;
}

void Body::cleanup() {
    auto sim = simulation_.lock();
    if(sim) {
        sim->release_body(this);
    }
}

void Body::move_to(const Vec3& position) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    auto xform = sim->body_transform(this);
    sim->set_body_transform(
        this,
        position,
        xform.second
    );
}

void Body::update(float dt) {
    const bool INTERPOLATION_ENABLED = true;

    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    if(INTERPOLATION_ENABLED) {
        auto prev_state = last_state_;
        auto next_state = sim->body_transform(this);

        float t = sim->time_keeper_->fixed_step_remainder() / dt;

        if(t < 0.0f) t = 0.0f;
        if(t > 1.0f) t = 1.0f;

        auto new_pos = prev_state.first + ((next_state.first - prev_state.first) * t);
        auto new_rot = prev_state.second.slerp(next_state.second, t);

        object_->move_to_absolute(new_pos);
        object_->rotate_to_absolute(new_rot);

        last_state_ = next_state;
    } else {
        auto state = sim->body_transform(this);
        object_->move_to_absolute(state.first);
        object_->rotate_to_absolute(state.second);
    }
}

void Body::build_collider(ColliderType collider) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    if(collider == COLLIDER_TYPE_BOX) {
        BoundableEntity* entity = dynamic_cast<BoundableEntity*>(object_);
        if(entity) {
            AABB aabb = entity->aabb();

            auto def = std::make_shared<b3BoxHull>();
            def->Set(aabb.width() * 0.5, aabb.height() * 0.5, aabb.depth() * 0.5);
            hulls_.push_back(def);

            b3HullShape hsdef;
            hsdef.m_hull = def.get();

            b3ShapeDef sdef;
            sdef.shape = &hsdef;
            sdef.userData = this;
            sdef.density = 0.005;
            sdef.friction = 0.3;

            sim->bodies_.at(this)->CreateShape(sdef);
        }
    } else if(collider == COLLIDER_TYPE_RAYCAST_ONLY) {
        assert(!is_dynamic()); // You can't have dynamic raycast colliders (yet)

        Actor* actor = dynamic_cast<Actor*>(object_);
        assert(actor && "Raycast colliders must be actors (or geoms, but that's not implemented");
        assert(actor->mesh_id());

        MeshPtr mesh = actor->stage->assets->mesh(actor->mesh_id());

        RaycastCollider* collider = &sim->raycast_colliders_[this];
        collider->triangles.clear();
        collider->vertices.clear();

        mesh->each([=](const std::string& name, SubMesh* submesh) {
            assert(submesh->arrangement() == MESH_ARRANGEMENT_TRIANGLES);

            uint32_t offset = collider->vertices.size();

            for(uint32_t i = 0; i < submesh->vertex_data->count(); ++i) {
                collider->vertices.push_back(submesh->vertex_data->position_at<Vec3>(i));
            }

            for(uint32_t i = 0; i < submesh->index_data->count(); i +=3) {
                Triangle tri;
                for(uint32_t j = 0; j < 3; ++j) {
                    tri.index[j] = offset + submesh->index_data->at(i + j);
                }

                auto v1 = collider->vertices[tri.index[1]] - collider->vertices[tri.index[0]];
                auto v2 = collider->vertices[tri.index[2]] - collider->vertices[tri.index[0]];
                tri.normal = v1.cross(v2).normalized();
                collider->triangles.push_back(tri);
            }
        });

        // Build the octree for performance
        collider->build_octree();
    }
}

}



}
}
