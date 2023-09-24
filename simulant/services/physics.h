#pragma once

#include "service.h"
#include "../types.h"

/* FIXME: hide! */
class b3Fixture;

namespace smlt {

typedef struct _PhysicsData PhysicsData;

class PhysicsBody;
class PhysicsService;
class PhysicsMaterial;

struct RayCastResult {
    PhysicsBody* other_body = nullptr;

    float distance = std::numeric_limits<float>::infinity();
    smlt::Vec3 normal;
    smlt::Vec3 impact_point;
};

class Fixture {
private:
    Fixture(PhysicsService* sim, b3Fixture* fixture);

    PhysicsBody* body_ = nullptr;
    uint16_t kind_ = 0;
public:
    friend class PrivateContactFilter;

    const PhysicsBody* body() const {
        return body_;
    }

    uint16_t kind() const {
        return kind_;
    }
};

class ContactFilter {
public:
    virtual bool should_collide(const Fixture* lhs, const Fixture* rhs) const = 0;
    virtual bool should_respond(const Fixture* lhs, const Fixture* rhs) const {
        _S_UNUSED(lhs);
        _S_UNUSED(rhs);
        return true;
    }
};

class PhysicsService:
    public Service {

public:
    smlt::optional<RayCastResult> ray_cast(
        const Vec3& start,
        const Vec3& direction,
        float max_distance=std::numeric_limits<float>::max()
    );

    void set_gravity(const Vec3& gravity);
    bool body_exists(const PhysicsBody* body) const;

    const ContactFilter* contact_filter() const;
    void set_contact_filter(ContactFilter* filter);

private:
    friend class PhysicsBody;
    friend class StaticBody;
    friend class ContactListener;
    friend class DynamicPhysicsBody;

    std::shared_ptr<PhysicsData> pimpl_;

    /* Accessible to PhysicsBody */

    /* Return the internal representaiton of a PhysicsBody. This
     * is a void* to avoid leaking the implementation */
    void* private_body(const PhysicsBody* body) const;

    void register_body(PhysicsBody* body);
    void unregister_body(PhysicsBody* body);

    void add_box_collider(
        PhysicsBody* self,
        const Vec3 &size, const PhysicsMaterial &properties, uint16_t kind, const Vec3 &offset, const Quaternion &rotation
    );

    void add_sphere_collider(
        PhysicsBody* self,
        const float diameter, const PhysicsMaterial& properties, uint16_t kind, const Vec3& offset
    );

    void add_triangle_collider(
        PhysicsBody* self,
        const smlt::Vec3& v1, const smlt::Vec3& v2, const smlt::Vec3& v3,
        const PhysicsMaterial& properties, uint16_t kind
    );

    void add_capsule_collider(
        PhysicsBody* self,
        float height, const float diameter, const PhysicsMaterial& properties, uint16_t kind
    );

    void add_mesh_collider(
        PhysicsBody* self,
        const MeshPtr& mesh, const PhysicsMaterial& properties,
        uint16_t kind, const Vec3& offset, const Quaternion& rotation
    );

    Vec3 body_position(const PhysicsBody* self) const;
    Quaternion body_rotation(const PhysicsBody* self) const;
};

}
