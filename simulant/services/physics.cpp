#include "physics.h"
#include "../meshes/mesh.h"
#include "../nodes/physics/joints.h"
#include "../nodes/physics/physics_body.h"
#include "../nodes/physics/private.h"
#include "../time_keeper.h"
#include "../utils/mesh/triangulate.h"
#include "bounce/bounce.h"
#include "bounce/collision/shapes/mesh_shape.h"
#include "simulant/macros.h"
#include <set>

/* Need for bounce */
void b3BeginProfileScope(const char* name) {
    _S_UNUSED(name);
}

void b3EndProfileScope() {

}


namespace smlt {

class ContactListener;
class PrivateContactListener;
class PrivateContactFilter;
class b3MeshGenerator;

class DebugDraw: public b3Draw {
public:
    DebugDraw(Debug* debug) :
        debug_(debug) {}

    Vec3 _p(const b3Vec3& p) const {
        return Vec3(p.x, p.y, p.z);
    }

    Color _c(const b3Color& c) const {
        return Color(c.r, c.g, c.b, c.a);
    }

    // Draw a point.
    virtual void DrawPoint(const b3Vec3& p, scalar size, const b3Color& color,
                           bool depthEnabled = true) {

        debug_->set_point_size(size);
        debug_->draw_point(_p(p), _c(color), Seconds(), depthEnabled);
    }

    // Draw a line segment.
    virtual void DrawSegment(const b3Vec3& p1, const b3Vec3& p2,
                             const b3Color& color, bool depthEnabled = true) {
        debug_->draw_line(_p(p1), _p(p2), _c(color), Seconds(), depthEnabled);
    }

    // Draw a triangle with vertices ordered CCW.
    virtual void DrawTriangle(const b3Vec3& p1, const b3Vec3& p2,
                              const b3Vec3& p3, const b3Color& color,
                              bool depthEnabled = true) {

        DrawSegment(p1, p2, color, depthEnabled);
        DrawSegment(p2, p3, color, depthEnabled);
        DrawSegment(p3, p1, color, depthEnabled);
    }

    // Draw a solid triangle with vertices ordered CCW.
    virtual void DrawSolidTriangle(const b3Vec3& normal, const b3Vec3& p1,
                                   const b3Vec3& p2, const b3Vec3& p3,
                                   const b3Color& color,
                                   bool depthEnabled = true) {
        _S_UNUSED(normal);
        DrawTriangle(p1, p2, p3, color, depthEnabled);
    }

    // Draw a polygon with vertices ordered CCW.
    virtual void DrawPolygon(const void* vertices, uint32 vertexStride,
                             uint32 count, const b3Color& color,
                             bool depthEnabled = true) {

        _S_UNUSED(vertices);
        _S_UNUSED(vertexStride);
        _S_UNUSED(count);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawPolygon unimplemented");
    }

    // Draw a solid polygon with vertices ordered CCW.
    virtual void DrawSolidPolygon(const b3Vec3& normal, const void* vertices,
                                  uint32 vertexStride, uint32 count,
                                  const b3Color& color,
                                  bool depthEnabled = true) {
        _S_UNUSED(normal);
        _S_UNUSED(vertices);
        _S_UNUSED(vertexStride);
        _S_UNUSED(count);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawSolidPolygon unimplemented");
    }

    // Draw a circle with center, normal, and radius.
    virtual void DrawCircle(const b3Vec3& normal, const b3Vec3& center,
                            scalar radius, const b3Color& color,
                            bool depthEnabled = true) {
        _S_UNUSED(normal);
        _S_UNUSED(center);
        _S_UNUSED(radius);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawCircle unimplemented");
    }

    // Draw a solid circle with center, normal, and radius.
    virtual void DrawSolidCircle(const b3Vec3& normal, const b3Vec3& center,
                                 scalar radius, const b3Color& color,
                                 bool depthEnabled = true) {

        _S_UNUSED(normal);
        _S_UNUSED(center);
        _S_UNUSED(radius);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawSolidCircle unimplemented");
    }

    // Draw a plane with center, normal and radius.
    virtual void DrawPlane(const b3Vec3& normal, const b3Vec3& center,
                           scalar radius, const b3Color& color,
                           bool depthEnabled = true) {
        _S_UNUSED(normal);
        _S_UNUSED(center);
        _S_UNUSED(radius);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawPlane unimplemented");
    }

    // Draw a solid plane with center, normal and radius.
    virtual void DrawSolidPlane(const b3Vec3& normal, const b3Vec3& center,
                                scalar radius, const b3Color& color,
                                bool depthEnabled = true) {
        _S_UNUSED(normal);
        _S_UNUSED(center);
        _S_UNUSED(radius);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawSolidPlane unimplemented");
    }

    // Draw a sphere with center, and radius.
    virtual void DrawSphere(const b3Vec3& center, scalar radius,
                            const b3Color& color, bool depthEnabled = true) {

        _S_UNUSED(center);
        _S_UNUSED(radius);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawSphere unimplemented");
    }

    // Draw a solid sphere with axis of rotation, center, and radius.
    virtual void DrawSolidSphere(const b3Vec3& axis, const b3Vec3& center,
                                 scalar radius, const b3Color& color,
                                 bool depthEnabled = true) {
        _S_UNUSED(axis);
        _S_UNUSED(center);
        _S_UNUSED(radius);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawSolidSphere unimplemented");
    }

    // Draw a cylinder with axis, center, radius, and height.
    virtual void DrawCylinder(const b3Vec3& axis, const b3Vec3& center,
                              scalar radius, scalar height,
                              const b3Color& color, bool depthEnabled = true) {

        _S_UNUSED(axis);
        _S_UNUSED(center);
        _S_UNUSED(radius);
        _S_UNUSED(height);
        _S_UNUSED(color);
        _S_UNUSED(depthEnabled);
        S_ERROR_ONCE("DrawCylinder unimplemented");
    }

    // Draw a solid cylinder with axis, center, radius, and height.
    virtual void DrawSolidCylinder(const b3Vec3& axis, const b3Vec3& center,
                                   scalar radius, scalar height,
                                   const b3Color& color,
                                   bool depthEnabled = true) {
        S_ERROR_ONCE("DrawSolidCylinder unimplemented");
    }

    // Draw a grid with orientation, center, width, and height.
    virtual void DrawGrid(const b3Vec3& normal, const b3Vec3& center,
                          uint32 width, uint32 height, const b3Color& color,
                          bool depthEnabled = true) {
        S_ERROR_ONCE("DrawGrid unimplemented");
    }

    // Draw a capsule with segment and radius.
    virtual void DrawCapsule(const b3Vec3& p1, const b3Vec3& p2, scalar radius,
                             const b3Color& color, bool depthEnabled = true) {
        S_ERROR_ONCE("DrawCapsule unimplemented");
    }

    // Draw a solid capsule with axis, segment and radius.
    virtual void DrawSolidCapsule(const b3Vec3& axis, const b3Vec3& p1,
                                  const b3Vec3& p2, scalar radius,
                                  const b3Color& color,
                                  bool depthEnabled = true) {
        S_ERROR_ONCE("DrawSolidCapsule unimplemented");
    }

    // Draw a AABB.
    virtual void DrawAABB(const b3AABB& aabb, const b3Color& color,
                          bool depthEnabled = true) {
        S_ERROR_ONCE("DrawAABB unimplemented");
    }

    // Draw a transform.
    virtual void DrawTransform(const b3Transform& xf,
                               bool depthEnabled = true) {}

private:
    Debug* debug_;
};

struct _PhysicsData {
    ContactFilter* filter_ = nullptr;
    std::weak_ptr<bool> filter_alive_;

    TimeKeeper* time_keeper_ = nullptr;

    std::shared_ptr<b3World> scene_;
    std::shared_ptr<ContactListener> contact_listener_;

    std::shared_ptr<PrivateContactFilter> contact_filter_;

    b3Body* get_b3body(const PhysicsBody* body) {
        return body->bounce_->body;
    }
};

class PrivateContactFilter : public b3ContactFilter {
public:
    PrivateContactFilter(PhysicsService* simulation):
        simulation_(simulation) {

    }

    bool ShouldCollide(b3Fixture *fixtureA, b3Fixture *fixtureB) override {
        auto filter = simulation_->contact_filter();

        if(!filter) {
            return true;
        }

        Fixture a((_impl::FixtureData*)fixtureA->GetUserData());
        Fixture b((_impl::FixtureData*)fixtureB->GetUserData());

        return filter->should_collide(&a, &b);
    }

    bool ShouldRespond(b3Fixture *fixtureA, b3Fixture *fixtureB) override {
        auto filter = simulation_->contact_filter();

        if(!filter) {
            return true;
        }

        Fixture a((_impl::FixtureData*)fixtureA->GetUserData());
        Fixture b((_impl::FixtureData*)fixtureB->GetUserData());

        return filter->should_respond(&a, &b);
    }

private:
    PhysicsService* simulation_ = nullptr;
};

class ContactListener : public b3ContactListener {
public:
    ContactListener(PhysicsService* simulation):
        simulation_(simulation) {

    }

    virtual ~ContactListener() {}

    void BeginContact(b3Contact* contact) {
        b3Fixture* fixtureA = contact->GetFixtureA();
        b3Fixture* fixtureB = contact->GetFixtureB();

        PhysicsBody* bodyA = (PhysicsBody*)fixtureA->GetBody()->GetUserData();
        PhysicsBody* bodyB = (PhysicsBody*)fixtureB->GetBody()->GetUserData();

        auto coll_pair = build_collision_pair(contact);
        auto& collA = coll_pair.first;
        auto& collB = coll_pair.second;

        bodyA->contact_started(collA);
        bodyB->contact_started(collB);

        // FIXME: Populate contact points

        active_contacts_.insert(contact);
    }

    void EndContact(b3Contact* contact) {
        if(active_contacts_.count(contact) == 0) {
            // Already released
            return;
        }

        b3Fixture* fixtureA = contact->GetFixtureA();
        b3Fixture* fixtureB = contact->GetFixtureB();

        PhysicsBody* bodyA = (PhysicsBody*)fixtureA->GetBody()->GetUserData();
        PhysicsBody* bodyB = (PhysicsBody*)fixtureB->GetBody()->GetUserData();

        auto coll_pair = build_collision_pair(contact);
        auto& collA = coll_pair.first;
        auto& collB = coll_pair.second;

        bodyA->contact_finished(collA);
        bodyB->contact_finished(collB);

        active_contacts_.erase(contact);
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

        PhysicsBody* bodyA = (PhysicsBody*)fixtureA->GetBody()->GetUserData();
        PhysicsBody* bodyB = (PhysicsBody*)fixtureB->GetBody()->GetUserData();

        Collision collA, collB;

        auto fA = (_impl::FixtureData*)fixtureA->GetUserData();
        auto fB = (_impl::FixtureData*)fixtureB->GetUserData();

        collA.this_body = bodyA;
        collA.this_collider_name = fA->name;

        collA.other_body = bodyB;
        collA.other_collider_name = fB->name;

        collB.other_body = bodyA;
        collB.other_collider_name = fA->name;

        collB.this_body = bodyB;
        collB.this_collider_name = fB->name;

        return std::make_pair(collA, collB);
    }

    std::set<b3Contact*> active_contacts_;
    PhysicsService* simulation_;
};

class b3MeshGenerator {
private:
    std::vector<b3Vec3> vertices_;
    std::vector<b3MeshTriangle> triangles_;
    std::shared_ptr<b3Mesh> mesh_;

public:
    b3MeshGenerator():
        mesh_(new b3Mesh()) {

    }

    void reserve_vertices(std::size_t count) {
        vertices_.reserve(count);
    }

    template<typename InputIterator>
    void insert_vertices(InputIterator first, InputIterator last) {
        auto count = std::distance(first, last);
        vertices_.reserve(vertices_.size() + count);

        for(auto it = first; it != last; ++it) {
            append_vertex((*it));
        }
    }

    void insert_triangles(
        const std::vector<utils::Triangle>::iterator first,
        const std::vector<utils::Triangle>::iterator last
    ) {

        b3MeshTriangle btri;

        auto count = std::distance(first, last);
        triangles_.reserve(triangles_.size() + count);

        for(auto it = first; it != last; ++it) {
            utils::Triangle& tri = (*it);

            btri.v1 = tri.idx[0];
            btri.v2 = tri.idx[1];
            btri.v3 = tri.idx[2];

            triangles_.push_back(btri);
            mesh_->triangles = &triangles_[0];
            mesh_->triangleCount = triangles_.size();
        }
    }

    void append_vertex(const Vec3& v) {
        b3Vec3 bv(v.x, v.y, v.z);
        vertices_.push_back(bv);

        mesh_->vertices = &vertices_[0];
        mesh_->vertexCount = vertices_.size();
    }

    b3Mesh* get_mesh() const { return mesh_.get(); }
};

const ContactFilter* PhysicsService::contact_filter() const {
    if(!pimpl_->filter_) {
        return nullptr;
    }

    /* Did the contact filter get destroyed? */
    if(!pimpl_->filter_alive_.lock()) {
        pimpl_->filter_alive_.reset();
        pimpl_->filter_ = nullptr;
    }

    return pimpl_->filter_;
}

void PhysicsService::set_contact_filter(ContactFilter* filter) {
    pimpl_->filter_ = filter;
    pimpl_->filter_alive_ = filter->alive_marker_;
}

void PhysicsService::register_body(PhysicsBody* body, const Vec3& pos, const Quaternion& rot) {
    b3BodyDef def;

    auto type = body->type();

    bool is_dynamic = false;

    if(type == PHYSICS_BODY_TYPE_KINEMATIC) {
        def.type = b3BodyType::e_kinematicBody;
        is_dynamic = true;
    } else if(type == PHYSICS_BODY_TYPE_DYNAMIC) {
        def.type = b3BodyType::e_dynamicBody;
        is_dynamic = true;
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
    def.position = b3Vec3(pos.x, pos.y, pos.z);
    def.orientation = b3Quat(rot.x, rot.y, rot.z, rot.w);

    body->bounce_->body = pimpl_->scene_->CreateBody(def);
}

void PhysicsService::unregister_body(PhysicsBody* body) {
    pimpl_->scene_->DestroyBody(body->bounce_->body);
}

void PhysicsService::init_sphere_joint(SphereJoint *joint, const ReactiveBody *a, const ReactiveBody *b, const Vec3 &a_off, const Vec3 &b_off) {
    auto b3a = a->bounce_->body;
    auto b3b = b->bounce_->body;

    if(!b3a || !b3b) {
        S_WARN("Couldn't create sphere joint as body was missing");
        return;
    }

    b3SphereJointDef def;
    def.bodyA = b3a;
    def.bodyB = b3b;
    def.collideConnected = false; // FIXME??
    def.localAnchorA.x = a_off.x;
    def.localAnchorA.y = a_off.y;
    def.localAnchorA.z = a_off.z;

    def.localAnchorB.x = b_off.x;
    def.localAnchorB.y = b_off.y;
    def.localAnchorB.z = b_off.z;

    joint->set_b3_joint(pimpl_->scene_->CreateJoint(def));
}

void PhysicsService::release_sphere_joint(SphereJoint *joint) {
    pimpl_->scene_->DestroyJoint((b3Joint*) joint->get_b3_joint());
    joint->set_b3_joint(nullptr);
}

void PhysicsService::add_box_collider(PhysicsBody* self, const Vec3& size,
                                      const PhysicsMaterial &properties, uint16_t kind, const Vec3 &offset, const Quaternion &rotation
) {
    b3Vec3 p(offset.x, offset.y, offset.z);
    b3Quat q(rotation.x, rotation.y, rotation.z, rotation.w);
    b3Transform tx(p, q);

    // Apply scaling
    b3Vec3 s;
    s.x = s.y = s.z = 1.0f;

    auto def = std::make_shared<b3BoxHull>(
        size.x * 0.5f, size.y * 0.5f, size.z * 0.5f
    );

    def->Transform(tx, s);
    self->bounce_->hulls.push_back(def);

    b3HullShape hsdef;
    hsdef.m_hull = def.get();

    b3FixtureDef sdef;
    sdef.shape = &hsdef;
    sdef.userData = this;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    auto fdata = std::make_shared<_impl::FixtureData>();
    fdata->fixture = self->bounce_->body->CreateFixture(sdef);
    fdata->fixture->SetUserData(fdata.get());
    fdata->material = properties;
    fdata->kind = kind;

    self->bounce_->fixtures.push_back(fdata);
}

void PhysicsService::add_sphere_collider(
    PhysicsBody* self,
    const float diameter, const PhysicsMaterial& properties, uint16_t kind, const Vec3& offset
) {
    b3SphereShape sphere;
    sphere.m_center = b3Vec3(offset.x, offset.y, offset.z);
    sphere.m_radius = diameter * 0.5f;

    b3FixtureDef sdef;
    sdef.shape = &sphere;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    auto fdata = std::make_shared<_impl::FixtureData>();
    fdata->fixture = self->bounce_->body->CreateFixture(sdef);
    fdata->fixture->SetUserData(fdata.get());
    fdata->material = properties;
    fdata->kind = kind;

    self->bounce_->fixtures.push_back(fdata);
}

void PhysicsService::add_triangle_collider(
    PhysicsBody* self,
    const smlt::Vec3& v1, const smlt::Vec3& v2, const smlt::Vec3& v3,
    const PhysicsMaterial& properties, uint16_t kind
) {
    b3Vec3 bv1 = b3Vec3(v1.x, v1.y, v1.z);
    b3Vec3 bv2 = b3Vec3(v2.x, v2.y, v2.z);
    b3Vec3 bv3 = b3Vec3(v3.x, v3.y, v3.z);

    b3TriangleShape tri;
    tri.Set(bv1, bv2, bv3);

    b3FixtureDef sdef;
    sdef.shape = &tri;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    auto fdata = std::make_shared<_impl::FixtureData>();
    fdata->fixture = self->bounce_->body->CreateFixture(sdef);
    fdata->fixture->SetUserData(fdata.get());
    fdata->material = properties;
    fdata->kind = kind;

    self->bounce_->fixtures.push_back(fdata);
}

void PhysicsService::add_capsule_collider(PhysicsBody* self, const Vec3& v0,
                                          const Vec3& v1, const float diameter,
                                          const PhysicsMaterial& properties,
                                          uint16_t kind) {

    b3CapsuleShape capsule;
    capsule.m_vertex1 = b3Vec3(v0.x, v0.y, v0.z);
    capsule.m_vertex2 = b3Vec3(v1.x, v1.y, v1.z);
    capsule.m_radius = diameter * 0.5f;

    b3FixtureDef sdef;
    sdef.shape = &capsule;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    auto fdata = std::make_shared<_impl::FixtureData>();
    fdata->fixture = self->bounce_->body->CreateFixture(sdef);
    fdata->fixture->SetUserData(fdata.get());
    fdata->material = properties;
    fdata->kind = kind;

    self->bounce_->fixtures.push_back(fdata);
}

void PhysicsService::add_mesh_collider(PhysicsBody* self, const MeshPtr& mesh,
                                       const PhysicsMaterial& properties,
                                       uint16_t kind, const Vec3& position,
                                       const Quaternion& orientation,
                                       const Vec3& scale) {
    auto bmesh = std::make_shared<b3MeshGenerator>();

    std::vector<utils::Triangle> triangles;

    bmesh->reserve_vertices(mesh->vertex_data->count());

    uint8_t* pos = mesh->vertex_data->data();
    auto stride = mesh->vertex_data->vertex_specification().stride();

    Mat4 tx = Mat4::as_transform(position, orientation, scale);

    for(std::size_t i = 0; i < mesh->vertex_data->count(); ++i, pos += stride) {
        auto p = tx * Vec4(*((Vec3*) pos), 1);
        bmesh->append_vertex(Vec3(p.x, p.y, p.z));
    }

    for(auto& submesh: mesh->each_submesh()) {
        submesh->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            utils::Triangle tri;
            tri.idx[0] = c;
            tri.idx[1] = b;
            tri.idx[2] = a;
            triangles.push_back(tri);
        });
    }

    // Add them to our b3Mesh generator
    bmesh->insert_triangles(triangles.begin(), triangles.end());

    // Build mesh AABB tree and mesh adjacency
    bmesh->get_mesh()->BuildTree();
    bmesh->get_mesh()->BuildAdjacency();

    // Grab the b3Mesh from the generator
    b3Mesh* genMesh = bmesh->get_mesh();

    b3MeshShape shape;
    shape.m_mesh = genMesh;

    b3FixtureDef sdef;
    sdef.shape = &shape;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    auto fdata = std::make_shared<_impl::FixtureData>();
    fdata->fixture = self->bounce_->body->CreateFixture(sdef);
    fdata->fixture->SetUserData(fdata.get());
    fdata->material = properties;
    fdata->kind = kind;
    fdata->mesh = bmesh;

    self->bounce_->fixtures.push_back(fdata);
}

void PhysicsService::on_fixed_update(float step) {
    uint32_t velocity_iterations = 8;
    uint32_t position_iterations = 2;
    pimpl_->scene_->Step(step, velocity_iterations, position_iterations);
}

void PhysicsService::on_update(float dt) {
    _S_UNUSED(dt);

    if(!debug_) {
        return;
    }

    auto draw = DebugDraw(debug_);

    for(b3Body* b = pimpl_->scene_->GetBodyList(); b; b = b->GetNext()) {
        for(b3Fixture* f = b->GetFixtureList(); f; f = f->GetNext()) {
            f->Draw(&draw, b3Color(1, 1, 1, 1));
        }
    }
}

void PhysicsService::set_gravity(const Vec3& gravity) {
    b3Vec3 g(gravity.x, gravity.y, gravity.z);
    pimpl_->scene_->SetGravity(g);
}

PhysicsService::PhysicsService():
    pimpl_(std::make_shared<PhysicsData>()) {

    pimpl_->scene_ = std::make_shared<b3World>();
    pimpl_->scene_->SetGravity(b3Vec3(0, -9.81, 0));

    pimpl_->contact_listener_ = std::make_shared<ContactListener>(this);
    pimpl_->contact_filter_ = std::make_shared<PrivateContactFilter>(this);

    pimpl_->scene_->SetContactListener(pimpl_->contact_listener_.get());
    pimpl_->scene_->SetContactFilter(pimpl_->contact_filter_.get());
}

PhysicsService::~PhysicsService() {
    // FIXME: Wipe all the bounce_ pointers on all physics objects
}

smlt::optional<RayCastResult> PhysicsService::ray_cast(const Vec3& start, const Vec3& direction, float max_distance) {
    b3RayCastSingleOutput result;
    b3Vec3 s(start.x, start.y, start.z);
    b3Vec3 d(direction.x, direction.y, direction.z);

    d *= max_distance;
    d += s;

    struct AlwaysCast : public b3RayCastFilter {
        bool ShouldRayCast(b3Fixture*) override {
            return true;
        }
    };

    AlwaysCast filter;

    bool hit = pimpl_->scene_->RayCastSingle(&result, &filter, s, d);

    float closest = std::numeric_limits<float>::max();

    if(hit) {
        RayCastResult ret;
        ret.other_body = (PhysicsBody*) (result.fixture->GetBody()->GetUserData());
        ret.impact_point = Vec3(result.point.x, result.point.y, result.point.z);
        closest = (ret.impact_point - start).length();
        ret.normal = Vec3(result.normal.x, result.normal.y, result.normal.z);
        ret.distance = closest;
        return smlt::optional<RayCastResult>(ret);
    }

    return smlt::optional<RayCastResult>();
}

}
