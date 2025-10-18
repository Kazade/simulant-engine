#pragma once

#include "../nodes/physics/fixture.h"
#include "../types.h"
#include "service.h"

namespace smlt {

typedef struct _PhysicsData PhysicsData;

class PhysicsBody;
class PhysicsService;
struct PhysicsMaterial;
class SphereJoint;
class ReactiveBody;

struct RayCastResult {
    PhysicsBody* other_body = nullptr;

    float distance = std::numeric_limits<float>::infinity();
    smlt::Vec3 normal;
    smlt::Vec3 impact_point;
};

class ContactFilter {
public:
    virtual bool should_collide(const Fixture* lhs, const Fixture* rhs) const = 0;
    virtual bool should_respond(const Fixture* lhs, const Fixture* rhs) const {
        _S_UNUSED(lhs);
        _S_UNUSED(rhs);
        return true;
    }

private:
    friend class PhysicsService;
    std::shared_ptr<bool> alive_marker_ = std::make_shared<bool>();
};

class PhysicsService:
    public Service {

public:
    PhysicsService();
    ~PhysicsService();

    smlt::optional<RayCastResult> ray_cast(
        const Vec3& start,
        const Vec3& direction,
        float max_distance=std::numeric_limits<float>::max()
    );

    void set_gravity(const Vec3& gravity);

    const ContactFilter* contact_filter() const;
    void set_contact_filter(ContactFilter* filter);

private:
    friend class PhysicsBody;
    friend class StaticBody;
    friend class ContactListener;
    friend class ReactiveBody;
    friend class Fixture;
    friend class SphereJoint;

    std::shared_ptr<PhysicsData> pimpl_;

    /* Accessible to PhysicsBody */
    void register_body(PhysicsBody* body, const Vec3& pos, const Quaternion& rot);
    void unregister_body(PhysicsBody* body);

    void init_sphere_joint(
        SphereJoint* joint,
        const ReactiveBody* a,
        const ReactiveBody* b,
        const Vec3& a_off,
        const Vec3& b_off
    );

    void release_sphere_joint(SphereJoint* joint);

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

    void add_capsule_collider(PhysicsBody* self, const Vec3& v0, const Vec3& v1,
                              const float diameter,
                              const PhysicsMaterial& properties, uint16_t kind);

    void add_mesh_collider(PhysicsBody* self, const MeshPtr& mesh,
                           const PhysicsMaterial& properties, uint16_t kind,
                           const Vec3& position = Vec3(),
                           const Quaternion& orientation = Quaternion(),
                           const Vec3& scale = Vec3(1));

    void on_fixed_update(float step) override;
};

}
