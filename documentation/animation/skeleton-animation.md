# Skeleton Animation

This document covers skeletal (skinned) animation in Simulant: how a skinned mesh is bound
to a hierarchy of joint nodes, and how those joints are driven at runtime.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Joints Are Scene Nodes](#2-joints-are-scene-nodes)
3. [Vertex Skinning](#3-vertex-skinning)
4. [Mesh::Skin](#4-meshskin)
5. [Instancing an Animated Prefab](#5-instancing-an-animated-prefab)
6. [Manipulating Joints at Runtime](#6-manipulating-joints-at-runtime)
7. [Constants and Limits](#7-constants-and-limits)
8. [Complete Example](#8-complete-example)

---

## 1. Overview

Skeleton animation deforms a mesh by driving a hierarchy of joints (bones). Each vertex in
a skinned mesh carries up to four joint indices and corresponding weights. When the joints
move, the vertices follow, producing smooth skeletal deformation.

Skinned meshes come from prefabs. Both the glTF (`.gltf`/`.glb`) and Milkshape3D (`.ms3d`)
loaders produce a `Prefab`: a template describing a tree of stage nodes, the meshes they
use, and any animations. Loading a prefab into the scene creates that tree, complete with
one node per joint.

```cpp
auto tree = scene->load_tree("models/character.glb");
tree->find_mixin<AnimationController>()->play("walk", ANIMATION_LOOP_FOREVER);
```

Headers:
- `simulant/assets/prefab.h` -- `Prefab`, `PrefabNode`, `PrefabAnimationChannel`
- `simulant/nodes/prefab_instance.h` -- `PrefabInstance`
- `simulant/nodes/animation_controller.h` -- `AnimationController`, `Animation`, `Channel`
- `simulant/meshes/mesh.h` -- `Mesh::Skin`, `Mesh::SkinBindPose`, `Mesh::update_skinning()`

---

## 2. Joints Are Scene Nodes

There is no dedicated skeleton or rig type. A joint is an ordinary `StageNode` in the
scene tree, and the joint hierarchy is just the node hierarchy:

```
PrefabInstance
  |-- AnimationController (mixin)
  |-- Actor           (holds the skinned mesh)
  |-- Stage "Root"    (joint 0)
        |-- Stage "Spine"   (joint 1)
              |-- Stage "Head"    (joint 2)
              |-- ...
```

This means the joints use the normal transform API, participate in the normal parenting
and dirty-transform machinery, and can be found with the usual lookups
(`find_descendent_with_name`, etc.). It also means anything can be parented to a joint --
attaching a weapon to a hand is just `weapon->set_parent(hand_joint)`.

Animations move joints by writing to their transforms. An `Animation` is a list of
`Channel`s, each pairing a target node with an `AnimationPath`
(`ANIMATION_PATH_TRANSLATION`, `ANIMATION_PATH_ROTATION`, `ANIMATION_PATH_SCALE`) and the
keyframe data for it. `AnimationController::on_update()` interpolates each channel at the
current time and calls `set_translation`/`set_rotation`/`set_scale_factor` on the target.

---

## 3. Vertex Skinning

A skinned mesh's vertex specification includes two extra attributes:

| Attribute | Type | Purpose |
|-----------|------|---------|
| `joint_attribute` | `VERTEX_ATTRIBUTE_4UB` or `VERTEX_ATTRIBUTE_4US` | Four joint indices |
| `weight_attribute` | `VERTEX_ATTRIBUTE_4F` or `VERTEX_ATTRIBUTE_4UB` | Four matching weights |

The joint indices index into `skin->node_indices`, not into the scene tree directly.

Skinning happens on the CPU in `Mesh::update_skinning()`, which the `AnimationController`
calls after applying each frame. For every vertex it builds a weighted blend of the joint
matrices and transforms the bind-pose position and normal by the result:

```
pre[j]       = inverse(actor_world) * joint_world[j] * inverse_bind[j]
skin_matrix  = sum(weight[k] * pre[joint[k]])   for k in 0..3
position_out = skin_matrix * rest_position
```

Vertices with no joint influence at all (total weight of zero - MS3D allows this) are left
at their bind-pose position rather than being collapsed onto the origin.

The bind-pose data itself (`Mesh::SkinBindPose`) is computed once per source mesh and
shared by pointer between instances, since it never changes with pose.

---

## 4. Mesh::Skin

```cpp
struct Mesh::Skin {
    std::vector<int16_t> joint_indices;          // Prefab node id per joint
    std::vector<StageNodePtr> node_indices;      // Resolved joint nodes
    std::vector<Mat4> inverse_bind_matrices;     // Inverse bind-pose transform per joint
    int8_t skeleton_root_node;                   // Prefab node id of the skeleton root
    StageNodePtr skeleton_root_stage_node;       // Resolved skeleton root
    ActorPtr bound_actor;                        // The Actor being skinned
};
```

`joint_indices` is filled in by the loader and refers to prefab node ids. `node_indices` is
resolved to actual scene nodes by `PrefabInstance` when the prefab is instantiated, so a
`Mesh::Skin` is only fully usable once it belongs to an instance.

`Mesh::is_skinned` tells you whether a mesh has skinning data at all.

---

## 5. Instancing an Animated Prefab

Skinning writes into a mesh's vertex data, so two instances of the same animated prefab
cannot share one mesh. `PrefabInstance::on_create()` handles that for you:

1. Build the node tree from the prefab.
2. For each skinned `Actor`, call `Mesh::create_skin_instance()` to give it a private mesh.
   The instance shares the bind-pose data and index data by pointer, and only gets its own
   (smaller) vertex buffer to receive skinning output.
3. Create an `AnimationController` mixin if the prefab has animations, and register each
   skinned mesh with it.
4. Resolve `skin->node_indices` and `skin->bound_actor` against the new nodes.

---

## 6. Manipulating Joints at Runtime

Because joints are scene nodes, posing them is just a transform write. Do it in a
`late_update()` so it runs after the `AnimationController` has applied the current frame:

```cpp
void on_late_update(float dt) override {
    auto neck = find_descendent_with_name("Neck");
    if(neck) {
        neck->transform->set_rotation(calculate_rotation_to_look());
    }
}
```

If you need the deformation to reflect the change immediately, call
`mesh->update_skinning()` afterwards.

---

## 7. Constants and Limits

| Limit | Value | Notes |
|-------|-------|-------|
| Joints per vertex | 4 | Fixed by the joint/weight vertex attributes |
| Joints per mesh | 255 (`4UB`) / 65535 (`4US`) | Depends on the joint attribute type |
| Prefab tree depth | 16 | `PrefabKey::path` is a `LimitedVector<uint32_t, 16>` |
| Interpolation | Linear | `ANIMATION_INTERPOLATION_STEP`/`CUBIC_SPLINE` currently fall back to linear |

---

## 8. Complete Example

```cpp
class GameScene : public smlt::Scene {
public:
    void on_load() override {
        auto tree = load_tree("models/character.glb");

        controller_ = tree->find_mixin<AnimationController>();
        controller_->play("idle", ANIMATION_LOOP_FOREVER);

        // Joints are just nodes, so a weapon can be parented to a hand
        auto hand = tree->find_descendent_with_name("Hand.R");
        auto sword = create_child<Actor>(assets->load_mesh("models/sword.obj"));
        sword->set_parent(hand);

        camera_ = create_child<Camera3D>();
        camera_->transform->set_position(Vec3(0, 1, 5));
        camera_->transform->look_at(Vec3(0, 0, 0));

        compositor->create_layer(this, camera_)
            ->set_clear_flags(BUFFER_CLEAR_ALL);
    }

    void start_walking() {
        controller_->play("walk", ANIMATION_LOOP_FOREVER);
    }

private:
    AnimationController* controller_ = nullptr;
    Camera3D* camera_ = nullptr;
};
```

### Key Takeaways

1. **Joints are scene nodes** -- pose them with the normal transform API, and parent
   things to them like any other node.
2. **Skinning is per-instance** -- `PrefabInstance` gives each skinned `Actor` its own mesh
   so instances don't fight over one vertex buffer.
3. **Weights should sum to 1.0** -- `update_skinning()` renormalises weights that sum to
   noticeably more than 1.0, and leaves unweighted vertices at their bind pose.
4. **Loading handles all of this** -- in practice you load a prefab and only interact with
   the `AnimationController` and the joint nodes.

---

## Related Documentation

- [Animation Controller](animation-controller.md) -- Playing and queueing animations
- [Animation System Overview](overview.md) -- High-level overview of all animation systems
- [Prefabs](../assets/prefabs.md) -- Loading animated models
- [Meshes](../rendering/meshes.md) -- Mesh creation and vertex specifications
- [Actors](../core-concepts/actors.md) -- Rendering entities in the scene
