# Skeleton Animation

This document covers skeletal (skinned) animation in Simulant: how a skinned mesh is bound
to a skeleton, and how that skeleton is driven at runtime.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Armatures and Joints](#2-armatures-and-joints)
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
an `Armature` and a `Joint` node per bone.

```cpp
auto tree = scene->load_tree("models/character.glb");
tree->find_mixin<AnimationController>()->play("walk", ANIMATION_LOOP_FOREVER);
```

Headers:
- `simulant/assets/prefab.h` -- `Prefab`, `PrefabNode`, `PrefabAnimationChannel`
- `simulant/nodes/prefab_instance.h` -- `PrefabInstance`
- `simulant/nodes/armature.h` -- `Armature`
- `simulant/nodes/joint.h` -- `Joint`
- `simulant/nodes/animation_controller.h` -- `AnimationController`, `Animation`, `Channel`
- `simulant/meshes/mesh.h` -- `Mesh::Skin`

---

## 2. Armatures and Joints

Two node types make up a skeleton:

- **`Armature`** is its root. It owns the skinned source mesh (or meshes) bound to the
  skeleton, owns the *posed* output mesh those are deformed into, and submits that output
  to the render queue. There's no `Actor` involved in rendering a skinned character.
- **`Joint`** is a single bone. Joints are ordinary stage nodes that happen to carry an
  index into the armature's inverse bind matrices.

```
PrefabInstance
  |-- AnimationController (mixin)
  |-- Armature "Rig"         (holds the skinned mesh, and renders it)
        |-- Joint "root"     (joint 0)
              |-- Joint "spine"  (joint 1)
                    |-- Joint "head"   (joint 2)
                    |-- ...
```

Joints use the normal transform API, participate in the normal parenting and
dirty-transform machinery, and can be found with the usual lookups
(`find_descendent_with_name`, or `Armature::find_joint`). It also means anything can be
parented to a joint -- attaching a weapon to a hand is just
`weapon->set_parent(hand_joint)`.

Animations move joints by writing to their transforms. An `Animation` is a list of
`Channel`s, each pairing a target node with an `AnimationPath`
(`ANIMATION_PATH_TRANSLATION`, `ANIMATION_PATH_ROTATION`, `ANIMATION_PATH_SCALE`) and the
keyframe data for it. `AnimationController::on_update()` interpolates each channel at the
current time, calls `set_translation`/`set_rotation`/`set_scale_factor` on the target, and
then re-poses every `Armature` below it.

A skeleton is posed in *armature space*, so where the armature sits in the scene doesn't
affect the pose -- moving the `Armature` moves the posed mesh with it. The only structural
requirement is that a skeleton's joints are descendents of its armature.

---

## 3. Vertex Skinning

A skinned mesh's vertex specification includes two extra attributes:

| Attribute | Type | Purpose |
|-----------|------|---------|
| `joint_attribute` | `VERTEX_ATTRIBUTE_4UB` or `VERTEX_ATTRIBUTE_4US` | Four joint indices |
| `weight_attribute` | `VERTEX_ATTRIBUTE_4F` or `VERTEX_ATTRIBUTE_4UB` | Four matching weights |

The joint indices index into the armature's joints -- `Armature::joint(i)` is the node for
joint index `i`, matched up by each `Joint`'s `joint_index`.

Skinning happens on the CPU in `Armature::update_skinning()`, which the
`AnimationController` calls after applying each frame. For every vertex it builds a
weighted blend of the joint matrices and transforms the bind-pose position and normal by
the result:

```
pre[j]       = inverse(armature_world) * joint_world[j] * inverse_bind[j]
skin_matrix  = sum(weight[k] * pre[joint[k]])   for k in 0..3
position_out = skin_matrix * rest_position
```

Vertices with no joint influence at all (total weight of zero - MS3D allows this) are left
at their bind-pose position rather than being collapsed onto the origin.

The bind pose is simply the source mesh's own vertex data, which is never written to. The
result goes into the armature's output mesh, which carries only what's needed to render
(position, normal, uv, color) and shares the source's index data by pointer, since
topology doesn't change with pose.

---

## 4. Mesh::Skin

```cpp
struct Mesh::Skin {
    std::vector<Mat4> inverse_bind_matrices;  // Inverse bind-pose transform per joint
};
```

That's the whole of it: how the mesh binds to a skeleton, and nothing about any particular
instance of one. It's filled in by the loader, shared by pointer between every mesh bound
to the same skeleton, and immutable thereafter.

`Mesh::is_skinned()` tells you whether a mesh has skinning data at all.

---

## 5. Instancing an Animated Prefab

Posing writes into vertex data, so two instances of the same animated prefab cannot share
one posed mesh. Because each `Armature` builds its own output mesh from the shared source,
this falls out for free -- `PrefabInstance` does nothing special:

1. Build the node tree from the prefab. Each `Armature` in it creates an output mesh per
   skinned mesh it was given.
2. Create an `AnimationController` mixin if the prefab has animations.

The immutable parts -- the bind pose, the inverse bind matrices, the index data and the
materials -- stay shared between instances.

A glTF skin can be referenced by any number of mesh nodes (it's common for a character to
be split into body/head/limb meshes sharing one rig). Those all end up on a single
`Armature`, which poses and renders each of them; `Armature::mesh_count()`,
`source_mesh(i)` and `skinned_mesh(i)` walk them.

---

## 6. Manipulating Joints at Runtime

Because joints are scene nodes, posing them is just a transform write. Do it in a
`late_update()` so it runs after the `AnimationController` has applied the current frame:

```cpp
void on_late_update(float dt) override {
    auto neck = armature_->find_joint("neck");
    if(neck) {
        neck->transform->set_rotation(calculate_rotation_to_look());
    }
}
```

Moving a joint marks its armature's pose dirty, so the deformation is refreshed before the
next render without you doing anything. If you need it to reflect the change *immediately*
-- to read the deformed vertices back, say -- call `armature->update_skinning()`.

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

        armature_ = static_cast<ArmaturePtr>(
            tree->find_descendents_by_types({Armature::Meta::node_type})[0]);

        // Joints are just nodes, so a weapon can be parented to a hand
        auto sword = create_child<Actor>(assets->load_mesh("models/sword.obj"));
        sword->set_parent(armature_->find_joint("hand.r"));

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
    Armature* armature_ = nullptr;
    Camera3D* camera_ = nullptr;
};
```

### Key Takeaways

1. **The `Armature` renders the character** -- not an `Actor`. It owns the skeleton, the
   posed mesh, and the renderables produced from it.
2. **Joints are scene nodes** -- pose them with the normal transform API, and parent
   things to them like any other node.
3. **Meshes are source data** -- a skinned `Mesh` only ever holds the bind pose, so it can
   be shared by any number of armatures.
4. **Weights should sum to 1.0** -- skinning renormalises weights that sum to noticeably
   more than 1.0, and leaves unweighted vertices at their bind pose.
5. **Loading handles all of this** -- in practice you load a prefab and only interact with
   the `AnimationController`, the `Armature` and the joint nodes.

---

## Related Documentation

- [Animation Controller](animation-controller.md) -- Playing and queueing animations
- [Animation System Overview](overview.md) -- High-level overview of all animation systems
- [Prefabs](../assets/prefabs.md) -- Loading animated models
- [Meshes](../rendering/meshes.md) -- Mesh creation and vertex specifications
- [Actors](../core-concepts/actors.md) -- Rendering entities in the scene
