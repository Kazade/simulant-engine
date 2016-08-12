#include "rigid_body.h"
#include "../object.h"
#include "../actor.h"
#include "../stage.h"

namespace kglt {
namespace controllers {

q3Vec3 to_q3vec3(const Vec3& rhs) {
    q3Vec3 ret;
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
    return ret;
}

Vec3 to_vec3(const q3Vec3& rhs) {
    Vec3 ret;
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
    return ret;
}

Mat3 from_q3mat3(const q3Mat3& rhs) {
    Mat3 ret;
    ret.mat[0] = rhs.Column0().x;
    ret.mat[3] = rhs.Column0().y;
    ret.mat[6] = rhs.Column0().z;

    ret.mat[1] = rhs.Column1().x;
    ret.mat[4] = rhs.Column1().y;
    ret.mat[7] = rhs.Column1().z;

    ret.mat[2] = rhs.Column2().x;
    ret.mat[5] = rhs.Column2().y;
    ret.mat[8] = rhs.Column2().z;

    return ret;
}

RigidBodySimulation::RigidBodySimulation() {
    scene_ = new q3Scene(1.0 / 60.0f);
    scene_->SetAllowSleep(true);
    scene_->SetGravity(q3Vec3(0, -9.81, 0));
}

bool RigidBodySimulation::init() {

    return true;
}

void RigidBodySimulation::cleanup() {

}



void RigidBodySimulation::step(double dt) {
    scene_->Step();
}

class Raycast : public q3QueryCallback {
public:
    q3RaycastData data;
    r32 tfinal;
    q3Vec3 nfinal;
    q3Body *impactBody;

    bool ReportShape( q3Box *shape ) {
        if ( data.toi < tfinal )
        {
            tfinal = data.toi;
            nfinal = data.normal;
            impactBody = shape->body;
        }

        data.toi = tfinal;
        return true;
    }

    void Init( const q3Vec3& spot, const q3Vec3& dir ) {
        data.start = spot;
        data.dir = q3Normalize( dir );
        data.t = r32( 10000.0 );
        tfinal = FLT_MAX;
        data.toi = data.t;
        impactBody = NULL;
    }
};

std::pair<Vec3, bool> RigidBodySimulation::intersect_ray(const Vec3& start, const Vec3& direction, float* distance) {
    bool hit = false;

    Raycast raycast;
    raycast.Init(to_q3vec3(start), to_q3vec3(direction));
    scene_->RayCast(&raycast, raycast.data);

    float closest = std::numeric_limits<float>::max();
    Vec3 impact_point;

    if(raycast.impactBody && raycast.data.toi <= direction.length()) {
        hit = true;
        closest = raycast.data.toi;
        impact_point = to_vec3(raycast.data.GetImpactPoint());
    }

    // Now, check all the raycast only colliders
    for(auto& p: raycast_colliders_) {
        float hit_distance;
        auto ret = p.second.intersect_ray(start, direction, &hit_distance);
        if(ret.second) {
            // We hit something
            if(hit_distance < closest) {
                closest = hit_distance;
                impact_point = ret.first;
                hit = true;
            }
        }
    }

    if(distance) {
        *distance = closest;
    }

    return std::make_pair(impact_point, hit);
}

q3Body* RigidBodySimulation::acquire_body(impl::Body *body) {
    q3BodyDef def;

    bool is_dynamic = body->is_dynamic();
    def.bodyType = (is_dynamic) ? eDynamicBody : eStaticBody;
    def.gravityScale = (is_dynamic) ? 1.0 : 0.0;

    bodies_[body] = scene_->CreateBody(def);
    return bodies_[body];
}

void RigidBodySimulation::release_body(impl::Body *body) {
    scene_->RemoveBody(bodies_.at(body));
}

std::pair<Vec3, Quaternion> RigidBodySimulation::body_transform(impl::Body *body) {
    q3Body* b = bodies_.at(body);
    auto xform = b->GetTransform();
    Mat3 rot = from_q3mat3(xform.rotation);

    return std::make_pair(
        Vec3(xform.position.x, xform.position.y, xform.position.z),
        Quaternion(rot)
    );
}

void RigidBodySimulation::set_body_transform(impl::Body* body, const Vec3& position, const Quaternion& rotation) {
    q3Body* b = bodies_.at(body);

    Vec3 axis;
    float angle;
    kmQuaternionToAxisAngle(&rotation, &axis, &angle);

    b->SetTransform(to_q3vec3(position), to_q3vec3(axis), angle);
}

RigidBody::RigidBody(Controllable* object, RigidBodySimulation::ptr simulation, ColliderType collider):
    Body(object, simulation, collider) {

}

RigidBody::~RigidBody() {

}

void RigidBody::add_force(const Vec3 &force) {
    q3Body* b = simulation_->bodies_.at(this);
    b->ApplyLinearForce(to_q3vec3(force));
}

void RigidBody::add_impulse(const Vec3& impulse) {
    q3Body* b = simulation_->bodies_.at(this);
    b->ApplyLinearImpulse(to_q3vec3(impulse));
}

void RigidBody::add_impulse_at_position(const Vec3& impulse, const Vec3& position) {
    q3Body* b = simulation_->bodies_.at(this);
    b->ApplyLinearImpulseAtWorldPoint(to_q3vec3(impulse), to_q3vec3(position));
}

void RigidBody::add_force_at_position(const Vec3& force, const Vec3& position) {
    q3Body* b = simulation_->bodies_.at(this);

    q3Vec3 f, p;

    f.x = force.x;
    f.y = force.y;
    f.z = force.z;

    p.x = position.x;
    p.y = position.y;
    p.z = position.z;

    b->ApplyForceAtWorldPoint(to_q3vec3(force), to_q3vec3(position));
}

void RigidBody::add_torque(const Vec3& torque) {
    q3Body* b = simulation_->bodies_.at(this);
    b->ApplyTorque(to_q3vec3(torque));
}

StaticBody::StaticBody(Controllable* object, RigidBodySimulation::ptr simulation, ColliderType collider):
    Body(object, simulation, collider) {

}

StaticBody::~StaticBody() {

}


namespace impl {

Body::Body(Controllable* object, RigidBodySimulation::ptr simulation, ColliderType collider_type):
    Controller("rigid-body"),
    simulation_(simulation),
    collider_type_(collider_type) {

    object_ = dynamic_cast<MoveableObject*>(object);
    if(!object_) {
        throw std::runtime_error("Tried to attach a rigid body controller to something that isn't moveable");
    }


}

Body::~Body() {

}

bool Body::init() {
    body_ = simulation_->acquire_body(this);
    build_collider(collider_type_);

    return true;
}

void Body::cleanup() {
    simulation_->release_body(this);
}

void Body::move_to(const Vec3& position) {
    auto xform = simulation_->body_transform(this);
    simulation_->set_body_transform(
        this,
        position,
        xform.second
    );
}

void Body::do_post_update(double dt) {
    auto xform = simulation_->body_transform(this);
    object_->set_absolute_position(xform.first);
    object_->set_absolute_rotation(xform.second);
}

void Body::build_collider(ColliderType collider) {
    if(collider == COLLIDER_TYPE_BOX) {
        BoundableEntity* entity = dynamic_cast<BoundableEntity*>(object_);
        if(entity) {
            AABB aabb = entity->aabb();

            q3BoxDef def;
            def.SetRestitution(0);

            q3Transform localSpace;
            q3Identity( localSpace );

            def.Set(localSpace, q3Vec3(aabb.width(), aabb.height(), aabb.depth()));
            simulation_->bodies_.at(this)->AddBox(def);
        }
    } else if(collider == COLLIDER_TYPE_RAYCAST_ONLY) {
        assert(!is_dynamic()); // You can't have dynamic raycast colliders (yet)

        Actor* actor = dynamic_cast<Actor*>(object_);
        assert(actor && "Raycast colliders must be actors (or geoms, but that's not implemented");
        assert(actor->mesh_id());

        MeshPtr mesh = actor->stage->assets->mesh(actor->mesh_id());

        RaycastCollider* collider = &simulation_->raycast_colliders_[this];
        collider->triangles.clear();
        collider->vertices.clear();

        mesh->each([=](SubMesh* submesh) {
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
