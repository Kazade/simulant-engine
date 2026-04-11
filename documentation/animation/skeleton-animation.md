# Skeleton Animation

This document covers the skeleton animation system in Simulant, including `Skeleton`, `Joint`, `Rig`, `RigJoint`, and vertex skinning.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Skeleton vs. Rig](#2-skeleton-vs-rig)
3. [Joint and Bone Hierarchy](#3-joint-and-bone-hierarchy)
4. [Vertex Skinning](#4-vertex-skinning)
5. [Creating a Skeleton Programmatically](#5-creating-a-skeleton-programmatically)
6. [Manipulating a Rig at Runtime](#6-manipulating-a-rig-at-runtime)
7. [SkeletalFrameUnpacker](#7-skeletalframeunpacker)
8. [Constants and Limits](#8-constants-and-limits)
9. [Complete Example: Procedural Skeleton](#9-complete-example-procedural-skeleton)

---

## 1. Overview

Skeleton animation in Simulant deforms a mesh by driving a hierarchy of joints (bones). Each vertex in a skinned mesh carries up to four joint indices and corresponding weights. When the joints move, the vertices follow, producing smooth skeletal deformation.

The system is split into two layers:

| Layer | Class | Purpose |
|-------|-------|---------|
| **Definition** | `Skeleton`, `Joint` | Rest-pose structure and vertex-to-joint weights (shared among instances) |
| **Runtime** | `Rig`, `RigJoint` | Per-instance pose data that changes every frame |

Headers:
- `simulant/assets/meshes/skeleton.h` -- `Skeleton`, `Joint`, `Bone`, `SkeletonFrame`, `SkeletonVertex`, `SkeletalFrameUnpacker`
- `simulant/assets/meshes/rig.h` -- `Rig`, `RigJoint`

---

## 2. Skeleton vs. Rig

A **Skeleton** is the definition -- it lives on a `Mesh` and is shared by every instance that uses that mesh. A **Rig** is a runtime copy of the skeleton's joint state. You manipulate the Rig to pose a character; the Skeleton defines what joints exist and how vertices attach to them.

```
Mesh
  |-- Skeleton (shared definition)
        |-- Joint 0  (Root)
        |-- Joint 1  (Spine)
        |-- Joint 2  (Head)
        |-- ...

Each animated instance gets its own:
  |-- Rig (per-instance runtime copy)
        |-- RigJoint 0
        |-- RigJoint 1
        |-- RigJoint 2
        |-- ...
```

Creating a Rig from a Skeleton:

```cpp
// skeleton is owned by the mesh
Skeleton* skeleton = mesh->skeleton.get();

// rig is a per-instance copy
auto rig = std::make_unique<Rig>(skeleton);
```

---

## 3. Joint and Bone Hierarchy

### Joint

A `Joint` represents a single bone in the skeleton hierarchy. Joint 0 is always the root.

```cpp
class Joint {
    std::string name() const;
    void set_name(const std::string& name);

    std::size_t id() const;          // Numeric index (0 = root)
    Joint* parent() const;           // Pointer to parent joint

    const Quaternion& rotation() const;       // Relative rotation
    const Vec3& translation() const;          // Relative translation
    const Quaternion& absolute_rotation() const;       // World-space rotation
    const Vec3 absolute_translation() const;           // World-space position

    void rotate_to(const Quaternion& q);
    void move_to(const Vec3& v);

    Bone* link_to(Joint* other);     // Create a bone link
};
```

Relative transforms are stored in `rotation()` and `translation()`. After modifying joints, call `recalc_absolute_transformation()` on the joint to compute world-space values (`absolute_rotation()` and `absolute_translation()`).

### Bone

A `Bone` is simply a link between two joints:

```cpp
struct Bone {
    Joint* joints[2] = {nullptr, nullptr};
};
```

Bones are created via `Joint::link_to()`:

```cpp
Joint* root = skeleton->joint(0);
Joint* spine = skeleton->joint(1);
root->link_to(spine);  // Creates a bone from root to spine
```

### Skeleton

The `Skeleton` class stores the joint array and manages vertex attachments:

```cpp
class Skeleton {
    Joint* joint(std::size_t idx);
    Joint* find_joint(const std::string& name);
    std::size_t joint_count() const;

    void attach_vertex_to_joint(std::size_t joint_index,
                                std::size_t vertex_index,
                                float weight);
};
```

Joints are pre-allocated at skeleton creation time. Access them by index or search by name:

```cpp
// By index (fast)
Joint* root = skeleton->joint(0);

// By name (linear search)
Joint* head = skeleton->find_joint("Head");
```

---

## 4. Vertex Skinning

Each vertex in a skinned mesh stores up to 4 joint indices and weights. The struct is defined in `skeleton.h`:

```cpp
#define MAX_JOINTS_PER_VERTEX 4
#define MAX_JOINTS_PER_MESH 64

struct SkeletonVertex {
    int32_t joints[MAX_JOINTS_PER_VERTEX] = {-1, -1, -1, -1};
    float weights[MAX_JOINTS_PER_VERTEX] = {0, 0, 0, 0};
};
```

Key rules:
- Joint indices of `-1` mean "no influence"
- Weights should typically sum to `1.0` for correct deformation
- A single mesh can have at most 64 joints
- Each vertex can be influenced by at most 4 joints

### Attaching Vertices to Joints

```cpp
// Attach vertex 42 to joint 0 with weight 0.8
skeleton->attach_vertex_to_joint(0, 42, 0.8f);

// Attach the same vertex to joint 1 with weight 0.2
skeleton->attach_vertex_to_joint(1, 42, 0.2f);
```

The `SkeletalFrameUnpacker` also provides a convenience method:

```cpp
SkeletalFrameUnpacker* unpacker = ...;
unpacker->link_vertex_to_joint(vertex_index, joint_index, weight);
```

This automatically finds the next available joint slot for the vertex (returns `false` if all 4 slots are full).

---

## 5. Creating a Skeleton Programmatically

In most cases, skeletons are loaded from GLTF/GLB files automatically. However, you can build them manually:

```cpp
// Create a mesh with skinned vertex specification
auto mesh = assets->create_mesh(smlt::VertexSpecification::DEFAULT_SKINNED);

// The mesh has a Skeleton attached
Skeleton* skeleton = mesh->skeleton.get();

// Configure joints
Joint* root = skeleton->joint(0);
root->set_name("Root");

Joint* spine = skeleton->joint(1);
spine->set_name("Spine");

Joint* head = skeleton->joint(2);
head->set_name("Head");

// Build the hierarchy
root->link_to(spine);
spine->link_to(head);

// Attach vertices with weights
// (Vertex index 0 is influenced 80% by Root, 20% by Spine)
skeleton->attach_vertex_to_joint(0, 0, 0.8f);
skeleton->attach_vertex_to_joint(1, 0, 0.2f);

// Create a runtime Rig
auto rig = std::make_unique<Rig>(skeleton);

// Pose the rig
RigJoint* head_joint = rig->find_joint("Head");
if (head_joint) {
    head_joint->rotate_to(
        Quaternion::angle_axis(Degrees(15), Vec3::right())
    );
}

// Recalculate world-space transforms for all joints
rig->recalc_absolute_transformations();
```

> **Note:** `DEFAULT_SKINNED` is a `VertexSpecification` that includes joint indices and weights in the vertex layout. Using a non-skinned specification will cause skinning calculations to be skipped.

---

## 6. Manipulating a Rig at Runtime

### RigJoint

`RigJoint` is the runtime counterpart to `Joint`. It stores the current pose for one bone in an animated instance:

```cpp
class RigJoint {
    void rotate_to(const Quaternion& rotation);
    void move_to(const Vec3& translation);

    const Vec3& translation() const;
    const Quaternion& rotation() const;

    RigJoint* parent() const;
    std::string name() const;
};
```

### Rig

```cpp
class Rig {
    Rig(const Skeleton* skeleton);

    RigJoint* joint(std::size_t index);
    RigJoint* find_joint(const std::string& name);
    std::size_t joint_count() const;

    void recalc_absolute_transformations();
};
```

Access joints by index (fast) or name (search):

```cpp
// By index
RigJoint* spine = rig->joint(1);
spine->rotate_to(Quaternion::angle_axis(Degrees(30), Vec3::up()));

// By name
RigJoint* left_arm = rig->find_joint("LeftArm");
if (left_arm) {
    left_arm->move_to(Vec3(0.5f, 1.0f, 0.0f));
}
```

### Recalculating Absolute Transforms

After modifying joint rotations or translations, you must recalculate world-space transforms:

```cpp
rig->recalc_absolute_transformations();
```

This walks the joint hierarchy from the root, computing `absolute_rotation` and `absolute_translation` for every joint. The `SkeletalFrameUnpacker` uses these absolute values when computing vertex positions.

> **Important:** Always call `recalc_absolute_transformations()` after manipulating joints and before the mesh is rendered. If you skip this step, the mesh will deform using stale world-space data.

---

## 7. SkeletalFrameUnpacker

The `SkeletalFrameUnpacker` is responsible for interpolating between keyframes and computing final vertex positions. It inherits from `FrameUnpacker` and is invoked during the animation update cycle.

```cpp
class SkeletalFrameUnpacker : public FrameUnpacker {
    void unpack_frame(
        const uint32_t current_frame,
        const uint32_t next_frame,
        const float t,              // Interpolation factor (0.0 - 1.0)
        Rig* const rig,
        VertexData* const out,
        Debug* const debug = nullptr
    ) override;

    void set_joint_state_at_frame(std::size_t frame, std::size_t joint, JointState state);
    const JointState& joint_state_at_frame(std::size_t frame, std::size_t joint) const;

    bool link_vertex_to_joint(std::size_t vertex, std::size_t joint, float weight);
    const SkeletonVertex* vertex_at(std::size_t i);
    const std::vector<SkeletonVertex>& vertices() const;
};
```

### How It Works

1. **Interpolation**: Given `current_frame`, `next_frame`, and an interpolation factor `t`, the unpacker interpolates joint transforms between the two keyframes.
2. **Absolute Transform Calculation**: It recursively visits joints from the root, computing world-space rotations and translations.
3. **Vertex Deformation**: For each vertex, it blends the positions based on joint influences and weights, producing the final skinned vertex positions and normals.
4. **Debug Drawing**: If a `Debug` pointer is passed, it draws lines between connected joints for visualization.

### JointState

```cpp
struct JointState {
    Quaternion rotation;              // Relative to parent
    Vec3 translation;                 // Relative to parent
    Quaternion absolute_rotation;     // World-space
    Vec3 absolute_translation;        // World-space
};
```

### SkeletonFrame

A `SkeletonFrame` holds the state of all joints at a single keyframe:

```cpp
struct SkeletonFrame {
    std::vector<JointState> joints;
};
```

---

## 8. Constants and Limits

| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_JOINTS_PER_VERTEX` | 4 | Maximum joints that can influence a single vertex |
| `MAX_JOINTS_PER_MESH` | 64 | Maximum joints in a single skeleton |

These are compile-time limits baked into `SkeletonVertex` and the `Skeleton::bones_` array. Exceeding them will cause data loss or assertion failures.

---

## 9. Complete Example: Procedural Skeleton

This example builds a simple three-joint skeleton from scratch and poses it:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class SkeletonDemoScene : public Scene {
public:
    SkeletonDemoScene(Window* window) : Scene(window) {}

    void on_load() override {
        // Create a skinned mesh
        auto mesh = assets->create_mesh(VertexSpecification::DEFAULT_SKINNED);
        mesh->set_name("ProceduralSkeleton");

        Skeleton* skeleton = mesh->skeleton.get();

        // Set up joints
        Joint* root = skeleton->joint(0);
        root->set_name("Root");

        Joint* torso = skeleton->joint(1);
        torso->set_name("Torso");

        Joint* head = skeleton->joint(2);
        head->set_name("Head");

        // Link the hierarchy
        root->link_to(torso);
        torso->link_to(head);

        // Add some vertices and attach them to joints
        // (In practice, this data comes from imported models)
        skeleton->attach_vertex_to_joint(0, 0, 1.0f);   // Root owns vertex 0
        skeleton->attach_vertex_to_joint(1, 1, 0.5f);   // Torso influences vertex 1
        skeleton->attach_vertex_to_joint(0, 1, 0.5f);   // Root also influences vertex 1
        skeleton->attach_vertex_to_joint(2, 2, 1.0f);   // Head owns vertex 2

        // Create the rig
        rig_ = std::make_unique<Rig>(skeleton);

        // Create an actor to render the mesh
        actor_ = create_child<Actor>();
        actor_->set_mesh(mesh);

        // Set up camera
        camera_ = create_child<Camera3D>();
        camera_->set_perspective_projection(
            Degrees(45.0f), window->aspect_ratio(), 0.1f, 100.0f
        );
        camera_->transform->set_position(Vec3(0, 1, 5));
        camera_->transform->look_at(Vec3(0, 0, 0));

        compositor->create_layer(actor_, camera_)
            ->set_clear_flags(BUFFER_CLEAR_ALL);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        // Animate the head joint back and forth
        float angle = std::sin(time_) * 0.3f;
        RigJoint* head = rig_->find_joint("Head");
        if (head) {
            head->rotate_to(
                Quaternion::angle_axis(Radians(angle), Vec3::right())
            );
        }

        rig_->recalc_absolute_transformations();
        time_ += dt;
    }

private:
    std::unique_ptr<Rig> rig_;
    ActorPtr actor_;
    Camera3D* camera_ = nullptr;
    float time_ = 0.0f;
};
```

### Key Takeaways

1. **Skeleton is shared, Rig is per-instance** -- The Skeleton defines structure; the Rig holds the current pose.
2. **Joint hierarchy matters** -- Parent transforms propagate to children. Always call `recalc_absolute_transformations()` after modifying joints.
3. **Vertex weights sum to 1.0** -- For correct deformation, ensure the weights for a vertex add up to approximately 1.0.
4. **GLTF loading handles all of this automatically** -- In practice, you load skeletons from GLTF files and only interact with the Rig to manipulate poses.

---

## Related Documentation

- [Animation Controller](animation-controller.md) -- Playing and blending skeletal animations
- [Animation System Overview](overview.md) -- High-level overview of all animation systems
- [Prefabs](../assets/prefabs.md) -- Loading animated models from GLTF files
- [Meshes](../rendering/meshes.md) -- Mesh creation and vertex specifications
- [Actors](../core-concepts/actors.md) -- Rendering entities in the scene
