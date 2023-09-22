#include <set>
#include "bounce/bounce.h"
#include "bounce/collision/shapes/mesh_shape.h"
#include "physics.h"
#include "../time_keeper.h"
#include "../nodes/physics_body.h"
#include "../utils/mesh/triangulate.h"
#include "../meshes/mesh.h"

namespace smlt {

class ContactListener;
class PrivateContactListener;
class PrivateContactFilter;
class b3MeshGenerator;

struct FixtureData {
    b3Fixture* fixture = nullptr;
    PhysicsMaterial material;
    std::string name;
    uint16_t kind = 0;
    std::shared_ptr<b3MeshGenerator> mesh;
};

struct BodyData {
    b3Body* body = nullptr;
    std::vector<std::shared_ptr<b3Hull>> hulls;
    std::vector<FixtureData> fixtures;
};


struct _PhysicsData {
    ContactFilter* filter_ = nullptr;

    TimeKeeper* time_keeper_ = nullptr;

    std::shared_ptr<b3World> scene_;
    std::shared_ptr<ContactListener> contact_listener_;
    std::shared_ptr<PrivateContactFilter> contact_filter_;

    std::unordered_map<PhysicsBody*, BodyData> bodies_;

    const FixtureData* find_fixture(PhysicsBody* body, const b3Fixture* b3fixture) const {
        auto b = bodies_.find(body);
        if(b == bodies_.end()) {
            return nullptr;
        }

        for(auto& f: b->second.fixtures) {
            if(f.fixture == b3fixture) {
                return &f;
            }
        }

        return nullptr;
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

        Fixture a(simulation_, fixtureA);
        Fixture b(simulation_, fixtureB);

        return filter->should_collide(&a, &b);
    }

    bool ShouldRespond(b3Fixture *fixtureA, b3Fixture *fixtureB) override {
        auto filter = simulation_->contact_filter();

        if(!filter) {
            return true;
        }

        Fixture a(simulation_, fixtureA);
        Fixture b(simulation_, fixtureB);

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

        PhysicsBody* bodyA = (PhysicsBody*) fixtureA->GetUserData();
        PhysicsBody* bodyB = (PhysicsBody*) fixtureB->GetUserData();

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

        PhysicsBody* bodyA = (PhysicsBody*) fixtureA->GetUserData();
        PhysicsBody* bodyB = (PhysicsBody*) fixtureB->GetUserData();

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

        PhysicsBody* bodyA = (PhysicsBody*) fixtureA->GetUserData();
        PhysicsBody* bodyB = (PhysicsBody*) fixtureB->GetUserData();

        Collision collA, collB;

        auto fA = simulation_->pimpl_->find_fixture(bodyA, fixtureA);
        auto fB = simulation_->pimpl_->find_fixture(bodyB, fixtureB);

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

bool PhysicsService::body_exists(const PhysicsBody* body) const {
    return pimpl_->bodies_.count((PhysicsBody*) body);
}

const ContactFilter* PhysicsService::contact_filter() const {
    return pimpl_->filter_;
}

void PhysicsService::set_contact_filter(ContactFilter* filter) {
    pimpl_->filter_ = filter;
}

void PhysicsService::register_body(PhysicsBody* body) {
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

    BodyData data;
    data.body = pimpl_->scene_->CreateBody(def);

    pimpl_->bodies_.insert(std::make_pair(body, data));
}

void PhysicsService::unregister_body(PhysicsBody* body) {
    auto& body_data = pimpl_->bodies_.at(body);
    pimpl_->scene_->DestroyBody(body_data.body);
    pimpl_->bodies_.erase(body);
}

void PhysicsService::add_box_collider(PhysicsBody* self, const Vec3& size,
    const PhysicsMaterial &properties, uint16_t kind, const Vec3 &offset, const Quaternion &rotation
) {

    BodyData& data = pimpl_->bodies_.at(self);

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
    data.hulls.push_back(def);

    b3HullShape hsdef;
    hsdef.m_hull = def.get();

    b3FixtureDef sdef;
    sdef.shape = &hsdef;
    sdef.userData = this;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    FixtureData fdata;
    fdata.fixture = data.body->CreateFixture(sdef);
    fdata.fixture->SetUserData(self);
    fdata.material = properties;
    fdata.kind = kind;

    data.fixtures.push_back(fdata);
}

void PhysicsService::add_sphere_collider(
    PhysicsBody* self,
    const float diameter, const PhysicsMaterial& properties, uint16_t kind, const Vec3& offset
) {
    BodyData& data = pimpl_->bodies_.at(self);

    b3SphereShape sphere;
    sphere.m_center = b3Vec3(offset.x, offset.y, offset.z);
    sphere.m_radius = diameter * 0.5f;

    b3FixtureDef sdef;
    sdef.shape = &sphere;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    FixtureData fdata;
    fdata.fixture = data.body->CreateFixture(sdef);
    fdata.fixture->SetUserData(self);
    fdata.material = properties;
    fdata.kind = kind;

    data.fixtures.push_back(fdata);
}

void PhysicsService::add_triangle_collider(
    PhysicsBody* self,
    const smlt::Vec3& v1, const smlt::Vec3& v2, const smlt::Vec3& v3,
    const PhysicsMaterial& properties, uint16_t kind
) {
    BodyData& data = pimpl_->bodies_.at(self);

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

    FixtureData fdata;
    fdata.fixture = data.body->CreateFixture(sdef);
    fdata.fixture->SetUserData(self);
    fdata.material = properties;
    fdata.kind = kind;

    data.fixtures.push_back(fdata);
}

void PhysicsService::add_capsule_collider(
    PhysicsBody* self,
    float height, const float diameter, const PhysicsMaterial& properties, uint16_t kind
) {
    BodyData& data = pimpl_->bodies_.at(self);

    float off = (height - (diameter * 0.5f)) * 0.5f;
    b3Vec3 v1(0.0f, off, 0.0f);
    b3Vec3 v2(0.0f, -off, 0.0f);

    b3CapsuleShape capsule;
    capsule.m_vertex1 = v1;
    capsule.m_vertex2 = v2;
    capsule.m_radius = diameter * 0.5f;

    b3FixtureDef sdef;
    sdef.shape = &capsule;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    FixtureData fdata;
    fdata.fixture = data.body->CreateFixture(sdef);
    fdata.fixture->SetUserData(self);
    fdata.material = properties;
    fdata.kind = kind;

    data.fixtures.push_back(fdata);
}

void PhysicsService::add_mesh_collider(PhysicsBody* self, const MeshPtr& mesh, const PhysicsMaterial& properties, uint16_t kind, const Vec3& offset, const Quaternion& rotation) {
    BodyData& data = pimpl_->bodies_.at(self);

    auto bmesh = std::make_shared<b3MeshGenerator>();

    std::vector<utils::Triangle> triangles;

    bmesh->reserve_vertices(mesh->vertex_data->count());

    uint8_t* pos = mesh->vertex_data->data();
    auto stride = mesh->vertex_data->vertex_specification().stride();

    Mat4 tx(rotation, offset, Vec3(1));

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

    FixtureData fdata;
    fdata.fixture = data.body->CreateFixture(sdef);
    fdata.fixture->SetUserData(self);
    fdata.material = properties;
    fdata.kind = kind;
    fdata.mesh = bmesh;

    data.fixtures.push_back(fdata);
}


}
