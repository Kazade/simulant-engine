# Joints

Joints connect two physics bodies together, constraining their relative motion. Simulant provides the `SphereJoint` for creating ball-and-socket style connections. This guide covers how to create, configure, and manage joints.

**Related documentation:** [Physics Overview](overview.md), [Rigid Bodies](rigid-bodies.md), [Colliders](colliders.md).

---

## 1. What is a Joint?

A joint is a constraint between two `ReactiveBody` instances. It restricts how the bodies can move relative to each other. Joints are commonly used for:

- Ragdoll characters
- Chains and ropes
- Door hinges
- Vehicle suspensions
- Pendulums

Joints are managed by the `PhysicsService` and are automatically cleaned up when either connected body is destroyed.

---

## 2. SphereJoint

The `SphereJoint` (also known as a ball-and-socket joint) connects two bodies at anchor points, allowing the second body to rotate freely around the anchor within a specified radius.

### 2.1 Creating a SphereJoint

```cpp
// Create two bodies
auto anchor_body = anchor_actor->create_child<DynamicBody>();
anchor_body->add_sphere_collider(0.3f, PhysicsMaterial::iron());
anchor_body->set_mass(5.0f);

auto swinging_body = pendulum_actor->create_child<DynamicBody>();
swinging_body->add_box_collider(Vec3(0.2f, 1, 0.2f), PhysicsMaterial::wood());
swinging_body->set_mass(1.0f);

// Create the joint
auto joint = anchor_body->create_sphere_joint(
    swinging_body,                        // The other body to connect
    Vec3(0, 0, 0),                       // Anchor point relative to anchor_body
    Vec3(0, 0.5f, 0)                     // Anchor point relative to swinging_body
);
```

The joint is created on the `ReactiveBody` (which both `DynamicBody` and `KinematicBody` inherit from). The method signature is:

```cpp
SphereJoint* create_sphere_joint(
    ReactiveBody* other,
    const Vec3& this_relative_anchor,
    const Vec3& other_relative_anchor
);
```

### 2.2 Anchor Points

Anchor points are specified in **local body space**. This means the anchors move and rotate with their respective bodies.

```cpp
// Anchor at the top of the swinging body (local space)
Vec3 swing_anchor = Vec3(0, 0.5f, 0);

// Anchor at the bottom of the anchor body (local space)
Vec3 body_anchor = Vec3(0, -0.3f, 0);

auto joint = anchor_body->create_sphere_joint(
    swinging_body,
    body_anchor,     // Where on the anchor body
    swing_anchor     // Where on the swinging body
);
```

### 2.3 Joint Ownership

When you create a joint, it is owned by both connected bodies. The joint is stored as a `std::shared_ptr<SphereJoint>` in each body's internal joint list:

```cpp
class ReactiveBody {
private:
    std::vector<std::shared_ptr<SphereJoint>> sphere_joints_;
};
```

Each body tracks its joints:

```cpp
std::size_t count = body->sphere_joint_count();
```

---

## 3. Joint Lifecycle

### 3.1 Automatic Cleanup

When a body with joints is destroyed, all its joints are automatically cleaned up:

```cpp
// ReactiveBody::on_destroy() handles this:
// 1. Remove each joint from the other body's joint list
// 2. Clear (destroy) all joints, which unregisters them from PhysicsService
body->destroy();  // Joints are automatically cleaned up
```

### 3.2 Manual Joint Removal

Joints are reference-counted through `shared_ptr`. When all references to a joint are removed, it is destroyed and unregistered from the physics simulation:

```cpp
// If you have access to the joint pointer:
// (Note: the API currently doesn't expose the joint list publicly)
// The joint will be cleaned up when either body is destroyed
```

For most use cases, you don't need to manually manage joints. Simply destroying either connected body will clean up the joint.

---

## 4. Common Joint Patterns

### 4.1 Pendulum

```cpp
class PendulumBehaviour : public StageNode {
public:
    FindResult<DynamicBody> anchor = smlt::FindDescendent<DynamicBody>("Anchor", this);
    FindResult<DynamicBody> bob = smlt::FindDescendent<DynamicBody>("Bob", this);

    void on_load() override {
        // Set up anchor body (static-feeling, heavy)
        anchor->add_sphere_collider(0.2f, PhysicsMaterial::iron());
        anchor->set_mass(100.0f);  // Very heavy = barely moves
        anchor->lock_rotation(true, true, true);  // Don't rotate
        anchor->set_linear_damping(1.0f);  // Dampen any movement

        // Set up pendulum bob
        bob->add_box_collider(Vec3(0.1f, 0.3f, 0.1f), PhysicsMaterial::iron());
        bob->set_mass(1.0f);

        // Connect them
        anchor->create_sphere_joint(
            bob,
            Vec3(0, 0, 0),        // Center of anchor
            Vec3(0, 0.3f, 0)     // Top of bob
        );
    }
};
```

### 4.2 Ragdoll Chain

```cpp
class RagdollBehaviour : public StageNode {
public:
    void on_load() override {
        PhysicsMaterial mat = PhysicsMaterial::wood();

        // Create body segments
        auto head = create_body_segment(Vec3(0.3f, 0.3f, 0.3f), mat, Vec3(0, 2.5f, 0));
        auto torso = create_body_segment(Vec3(0.4f, 0.6f, 0.3f), mat, Vec3(0, 1.7f, 0));
        auto hips = create_body_segment(Vec3(0.35f, 0.3f, 0.25f), mat, Vec3(0, 1.1f, 0));

        // Connect head to torso
        torso->create_sphere_joint(
            head,
            Vec3(0, 0.6f, 0),     // Top of torso
            Vec3(0, -0.3f, 0)    // Bottom of head
        );

        // Connect torso to hips
        hips->create_sphere_joint(
            torso,
            Vec3(0, 0.3f, 0),     // Top of hips
            Vec3(0, -0.6f, 0)    // Bottom of torso
        );
    }

    DynamicBody* create_body_segment(Vec3 size, PhysicsMaterial mat, Vec3 pos) {
        auto actor = create_child<Actor>();
        auto mesh = assets->new_mesh_from_procedural_cube();
        actor->set_mesh(mesh->id());
        actor->scale_by(size.x * 2, size.y * 2, size.z * 2);
        actor->move_to(pos);

        auto body = actor->create_child<DynamicBody>();
        body->add_box_collider(size, mat);
        body->set_mass(1.0f);

        return body;
    }
};
```

### 4.3 Rope / Chain

```cpp
class RopeBehaviour : public StageNode {
public:
    FindResult<DynamicBody> anchor = smlt::FindDescendent<DynamicBody>("Anchor", this);

    void on_load() override {
        // Anchor point (fixed)
        anchor->add_sphere_collider(0.1f, PhysicsMaterial::iron());
        anchor->set_mass(1000.0f);
        anchor->lock_rotation(true, true, true);
        anchor->set_linear_damping(1.0f);

        // Create chain links
        int num_links = 10;
        float link_size = 0.2f;
        DynamicBody* previous = anchor;

        for (int i = 0; i < num_links; ++i) {
            auto link_actor = create_child<Actor>();
            auto mesh = assets->new_mesh_from_procedural_cube();
            link_actor->set_mesh(mesh->id());
            link_actor->scale_by(link_size, link_size, link_size);
            link_actor->move_to(0, 2.0f - i * link_size * 2, 0);

            auto link_body = link_actor->create_child<DynamicBody>();
            link_body->add_box_collider(
                Vec3(link_size / 2, link_size / 2, link_size / 2),
                PhysicsMaterial::wood()
            );
            link_body->set_mass(0.5f);

            // Connect to previous link
            previous->create_sphere_joint(
                link_body,
                Vec3(0, -link_size / 2, 0),   // Bottom of previous
                Vec3(0, link_size / 2, 0)     // Top of current
            );

            previous = link_body;
        }
    }
};
```

### 4.4 Door Hinge

```cpp
class DoorBehaviour : public StageNode {
public:
    FindResult<KinematicBody> door_frame = smlt::FindDescendent<KinematicBody>("Frame", this);
    FindResult<DynamicBody> door_panel = smlt::FindDescendent<DynamicBody>("Door", this);

    void on_load() override {
        // Door frame (kinematic = doesn't move from physics)
        door_frame->add_box_collider(Vec3(0.1f, 2, 0.1f), PhysicsMaterial::stone());

        // Door panel
        door_panel->add_box_collider(Vec3(0.05f, 1, 0.4f), PhysicsMaterial::wood());
        door_panel->set_mass(2.0f);

        // Hinge joint at the door edge
        door_frame->create_sphere_joint(
            door_panel,
            Vec3(0, 0, 0),         // Frame hinge point
            Vec3(0, 0, 0.4f)      // Door edge (hinge side)
        );
    }
};
```

---

## 5. PhysicsService and Joint Management

Joints are created and managed by the `PhysicsService`. When you call `create_sphere_joint()` on a body, the service initializes the underlying physics joint:

```cpp
// From joints.cpp:
SphereJoint::SphereJoint(ReactiveBody* a, ReactiveBody* b,
                         const Vec3& aoff, const Vec3& boff)
    : impl_(new SphereJointImpl()) {

    auto sim = a->scene->find_service<PhysicsService>();
    if (sim) {
        sim->init_sphere_joint(this, a, b, aoff, boff);
    }

    impl_->body_a_ = a;
    impl_->body_b_ = b;
}
```

When the joint is destroyed, it is released from the service:

```cpp
SphereJoint::~SphereJoint() {
    auto sim = impl_->body_a_->scene->find_service<PhysicsService>();
    if (sim) {
        sim->release_sphere_joint(this);
    }
}
```

### Internal Implementation

The joint system uses the **Bounce** physics library internally. The `SphereJoint` holds a pointer to the underlying `b3Joint`:

```cpp
struct SphereJointImpl {
    b3Joint* joint_ = nullptr;
    ReactiveBody* body_a_ = nullptr;
    ReactiveBody* body_b_ = nullptr;
};
```

---

## 6. Joint Limitations and Notes

### 6.1 Current Joint Type

Simulant currently provides only `SphereJoint`. This is a ball-and-socket joint that constrains the connected bodies to pivot around anchor points. Other joint types (hinge, slider, fixed, etc.) are not currently exposed in the public API.

### 6.2 Body Requirements

Both bodies connected to a joint must be `ReactiveBody` instances (i.e., `DynamicBody` or `KinematicBody`). `StaticBody` cannot participate in joints because it has no physics simulation state.

If you need a joint anchored to a fixed point, create a very heavy `DynamicBody` with locked rotation and high damping to act as an immovable anchor:

```cpp
auto anchor = create_child<DynamicBody>();
anchor->add_sphere_collider(0.1f, PhysicsMaterial::iron());
anchor->set_mass(1000.0f);
anchor->lock_rotation(true, true, true);
anchor->set_linear_damping(1.0f);
```

### 6.3 Joint Count Tracking

Each body tracks how many joints it participates in:

```cpp
std::size_t count = body->sphere_joint_count();
```

This is useful for debugging and for ensuring joints are properly cleaned up.

---

## 7. Debugging Joints

### 7.1 Visualizing Joints

Enable physics debug visualization to see joints rendered in the scene:

```cpp
auto debug = /* your Debug node */;
physics_service->set_debug(debug);
```

### 7.2 Checking Joint State

```cpp
// Check how many joints a body has
S_INFO("Body has {} joints", body->sphere_joint_count());

// Access joint body references
ReactiveBody* body_a = joint->first_body();
ReactiveBody* body_b = joint->second_body();
```

### 7.3 Common Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Joint doesn't form | One body has no PhysicsService | Ensure both bodies are in a scene with a `PhysicsService` |
| Bodies separate | Anchor points too far apart | Ensure anchor points are at the same world position when created |
| Joint doesn't clean up | Body destroyed without cleanup | Bodies auto-cleanup joints in `on_destroy()` |
| Bodies jitter | Mass ratio too extreme | Keep mass ratios under 10:1 for stable joints |

---

## See Also

- [Physics Overview](overview.md) -- General physics introduction
- [Rigid Bodies](rigid-bodies.md) -- Body types and properties
- [Colliders](colliders.md) -- Collider shapes and materials
- [Raycasting](raycasting.md) -- Querying the physics world
- [Best Practices](best-practices.md) -- Optimization and patterns
