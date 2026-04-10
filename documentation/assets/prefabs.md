# Prefab System

Prefabs let you load entire hierarchies of nodes from a file and instantiate them into your scene with a single call. They are the primary mechanism for importing 3D models, characters, levels, and any other pre-built scene content into a Simulant game.

**Related documentation:** [Stage Nodes](../core-concepts/stage-nodes.md), [Asset Managers](asset-managers.md), [Mesh Formats](mesh-formats.md).

---

## 1. What Are Prefabs and Why Use Them?

A **Prefab** is a template for a hierarchy of [StageNodes](../core-concepts/stage-nodes.md). Think of it as a blueprint that describes a tree of nodes -- their types, transforms, meshes, materials, animations, and other properties -- all stored in a file on disk.

A **PrefabInstance** is what actually places that blueprint into your scene. One prefab can be instantiated many times, each producing an independent copy of the node hierarchy.

### Why use prefabs?

- **Reuse**: Load a character model once, spawn it a hundred times.
- **Separation of content and code**: Artists build characters, props, and levels in tools like Blender and export them as files. Your code simply loads and positions them.
- **Hierarchical loading**: A single `load_prefab()` call can bring in an entire scene graph with actors, lights, cameras, and animations already wired together.
- **Animation support**: When you load an animated model (e.g., a rigged character), the prefab automatically creates an `AnimationController` mixin on the instance, ready for playback.
- **Nested composition**: Prefabs can contain any combination of node types -- actors, lights, particle systems, stages for grouping -- all in one file.

### Prefab vs. manually creating nodes

Without prefabs, building a character in code looks like this:

```cpp
// Manual approach -- lots of boilerplate
auto character = create_child<Stage>()->set_name_and_get("Character");
auto body_mesh = assets->load_mesh("body.obj");
auto body = character->create_child<Actor>(body_mesh);
body->set_name("Body");

auto head_mesh = assets->load_mesh("head.obj");
auto head = body->create_child<Actor>(head_mesh);
head->set_name("Head");
head->transform->set_position(Vec3(0, 1.5f, 0));

// ... many more lines for arms, legs, weapons, etc.
```

With a prefab, the same result is one line:

```cpp
auto instance = create_child<PrefabInstance>(
    assets->load_prefab("character.glb")
);
```

---

## 2. Loading Prefabs

Prefabs are loaded through the scene's `AssetManager` using `load_prefab()`:

```cpp
PrefabPtr prefab = assets->load_prefab("models/hero.glb");
```

### Parameters

`load_prefab()` accepts three arguments:

| Parameter | Type | Description |
|-----------|------|-------------|
| `filename` | `Path` | The path to the prefab file (e.g., `.gltf`, `.glb`) |
| `opts` | optional load options | Format-specific loading options (often omitted) |
| `gc_method` | `GarbageCollectMethod` | Garbage collection behavior (default: `GARBAGE_COLLECT_PERIODIC`) |

### Keeping prefabs in memory

By default, prefabs use periodic garbage collection. This means the prefab will be unloaded when no `PrefabInstance` references it. If you plan to spawn the same prefab repeatedly, keep it in memory by using `GARBAGE_COLLECT_NEVER`:

```cpp
PrefabPtr enemy_prefab = assets->load_prefab(
    "models/enemy.glb",
    PrefabLoadOptions(),
    GARBAGE_COLLECT_NEVER
);

// Later, when you are completely done with it:
enemy_prefab->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);
```

### Creating prefabs from scene nodes

You can also create a prefab **programmatically** from nodes already in your scene. This is useful for saving player-built structures or snapshotting a scene layout:

```cpp
// Suppose 'tree_group' is a StageNode in your scene
// create_prefab captures that node and all of its descendents
auto tree_prefab = assets->create_prefab(tree_group);

// tree_prefab now contains a snapshot of the hierarchy
// You can instantiate it later:
auto clone = scene->create_child<PrefabInstance>(tree_prefab);
```

### Finding prefabs by name

If you gave a prefab a name when loading it, you can look it up later:

```cpp
PrefabPtr found = assets->find_prefab("MyCharacter");
```

### Checking prefab existence and count

```cpp
if (assets->has_prefab(prefab_id)) {
    // The prefab is loaded
}

size_t total = assets->prefab_count();
```

---

## 3. GLTF/GLB as Prefabs

The primary prefab format in Simulant is **glTF 2.0** (both `.gltf` JSON and `.glb` binary formats). When you load a glTF file, the engine parses it and builds a `Prefab` internally.

### What glTF data becomes a prefab?

When a glTF file is loaded as a prefab, the following data is processed:

| glTF Element | Becomes a Prefab... |
|--------------|---------------------|
| Nodes | `PrefabNode` entries with type, name, and transform |
| Meshes | Loaded as `Mesh` assets, referenced by actor nodes |
| Materials | Loaded as `Material` assets |
| Textures | Loaded as `Texture` assets |
| Animations | `PrefabAnimationChannel` entries per animation |
| Skins | Skeletal data linked to actor meshes |
| Lights (KHR extension) | Light nodes with color, intensity, range |
| Extras (`s_node`) | Custom node type override |

### Node type mapping

The GLTF loader maps glTF nodes to Simulant stage node types:

- Nodes with a mesh become **Actor** nodes.
- Nodes with a camera definition become **Camera** nodes.
- Nodes with a light definition become **Light** nodes.
- All other nodes become **Stage** (grouping) nodes.

You can override the node type by setting the `s_node` property in the glTF node's `extras`:

```json
{
  "nodes": [{
    "name": "MyParticleEmitter",
    "extras": {
      "s_node": "ParticleSystem",
      "s_node_params": {
        "script": "my_particles.spt"
      }
    }
  }]
}
```

### Export tips for glTF

- **Y-Up orientation**: Simulant expects glTF files to be exported as Y-up. Make sure your 3D modeling tool is configured for Y-up export.
- **Skeletal animation**: glTF skeletal animations are fully supported. The loader creates an `AnimationController` mixin automatically.
- **IK constraints**: Inverse kinematics are **not** supported. Bake IK into keyframes before export.
- **Multiple scenes**: If a glTF file contains multiple scenes, you can specify which scene to load via load options.

### Embedded resources

GLB files embed meshes, textures, and materials in a single binary file. This is the recommended format for distribution because it is a single self-contained file. Separate `.gltf` + `.bin` + texture files also work but require all referenced files to be present.

For more details on supported formats, see [Mesh Formats](mesh-formats.md).

---

## 4. Instantiating Prefabs with PrefabInstance

A `PrefabInstance` is a [StageNode](../core-concepts/stage-nodes.md) subclass that takes a `Prefab` and materializes its node hierarchy as children.

### Basic instantiation

```cpp
// Step 1: Load the prefab
PrefabPtr character_prefab = assets->load_prefab("models/character.glb");

// Step 2: Create a PrefabInstance, passing the prefab
auto character = create_child<PrefabInstance>(character_prefab);
```

That is all it takes. The `PrefabInstance`:

1. Reads every `PrefabNode` from the prefab.
2. Creates the corresponding `StageNode` objects (Actors, Cameras, Lights, Stages, etc.).
3. Sets up the parent-child relationships to match the original hierarchy.
4. If the prefab has animations, attaches an `AnimationController` mixin.
5. If the prefab has skinned meshes, binds the skeleton to the joint nodes.

### The PrefabInstance as a container

The `PrefabInstance` itself acts as the root of the spawned hierarchy. All nodes from the prefab become children (or descendants) of the `PrefabInstance`. This means:

```cpp
// Move the entire character by moving the PrefabInstance
character->transform->set_position(Vec3(10, 0, 5));

// Rotate the entire character
character->transform->set_rotation(Quat::from_axis_angle(Vec3::UP, Degrees(90)));

// Hide the entire character
character->set_visible(false);
```

### Passing the prefab as a parameter

You can also pass the prefab through the `Params` system:

```cpp
Params params;
params.set("prefab", character_prefab);
auto character = create_child<PrefabInstance>(params);
```

### Error handling

If the prefab pointer is null when the `PrefabInstance` is created, an error is logged and instantiation fails:

```cpp
auto bad_prefab = PrefabPtr(); // null
auto instance = create_child<PrefabInstance>(bad_prefab);
// Logs: "Prefab was unexpectedly null"
```

---

## 5. Accessing Nodes in Instantiated Prefabs

Once a prefab is instantiated, you often need to reach into the hierarchy to access specific nodes -- for example, to attach a weapon to a character's hand, or to read a spawn point's position.

### Finding by name

```cpp
// Find a specific node by its name
StageNode* weapon_mount = character->find_descendent_with_name("WeaponMount");
if (weapon_mount) {
    // Attach something to the weapon mount
    auto gun = weapon_mount->create_child<Actor>(gun_mesh);
}
```

### Finding by type

Search for all nodes of a particular type:

```cpp
// Find all Actor nodes in the prefab instance
std::vector<StageNode*> actors =
    character->find_descendents_by_types({Actor::Meta::node_type});

for (auto* node : actors) {
    auto* actor = static_cast<Actor*>(node);
    printf("Actor: %s\n", actor->name().c_str());
}
```

### Finding the AnimationController

If the prefab has animations, an `AnimationController` mixin is attached to the `PrefabInstance`:

```cpp
auto anim_controller = character->find_mixin<AnimationController>();
if (anim_controller) {
    // List available animations
    auto names = anim_controller->animation_names();
    for (auto& name : names) {
        printf("Animation: %s\n", name.c_str());
    }

    // Play the first animation
    anim_controller->play(names[0]);
}
```

### Finding by unique ID

Every StageNode has a unique `StageNodeID`. If you know the ID, you can look it up directly:

```cpp
StageNode* node = character->find_descendent_with_id(some_id);
```

### Using FindResult for cached lookups

If you need to access a node repeatedly (e.g., every frame), use `FindResult<T>` to cache the lookup:

```cpp
class CharacterController : public StageNode {
public:
    FindResult<Actor> weapon_mount = FindDescendent<Actor>("WeaponMount", this);

    void on_update(float dt) override {
        if (weapon_mount) {
            // Cached after first access
            weapon_mount->transform->set_position(...);
        }
    }
};
```

### Traversing the hierarchy

You can iterate children and descendents directly:

```cpp
// Iterate immediate children
for (auto& child : character->each_child()) {
    printf("Child: %s\n", child.name().c_str());
}

// Iterate all descendents
for (auto& desc : character->each_descendent()) {
    printf("Node: %s (type: %s)\n", desc.name().c_str(), desc.node_type_name().c_str());
}
```

---

## 6. Modifying Prefabs After Instantiation

Each `PrefabInstance` is an independent copy. Changes you make to the instance do **not** affect the original prefab or other instances.

### Transform changes

```cpp
// Position the instance in the world
instance->transform->set_position(Vec3(0, 0, 0));

// Scale the entire instance
instance->transform->set_scale_factor(Vec3(2.0f, 2.0f, 2.0f));
```

### Modifying child nodes

```cpp
// Find a specific actor and change its material
auto* body = instance->find_descendent_with_name("Body");
if (body) {
    auto* actor = static_cast<Actor*>(body);
    actor->base_mesh()->first_submesh()->material()->set_lighting_enabled(false);
}
```

### Playing animations

```cpp
auto anim_controller = instance->find_mixin<AnimationController>();
if (anim_controller) {
    auto animations = anim_controller->animation_names();
    if (!animations.empty()) {
        anim_controller->play(animations[0]);           // Play immediately
        anim_controller->queue(animations[1]);          // Queue next
        anim_controller->queue(animations[2]);
    }
}
```

### Looping animations

```cpp
// Play an animation on loop (e.g., 999 repeats)
anim_controller->play("idle", 999);
```

### Adding children to the instance

You can add new nodes to a `PrefabInstance` just like any other `StageNode`:

```cpp
// Attach a particle effect to the character
auto particles = instance->create_child<ParticleSystem>(particle_script);
particles->transform->set_position(Vec3(0, 2, 0));
```

### Detaching nodes from the instance

```cpp
// Remove a child from the prefab instance
StageNode* child = instance->child_at(0);
child->remove_from_parent();
// The child now belongs to the root Stage
```

---

## 7. Nested Prefabs

A prefab can contain any type of StageNode, including other prefabs. This enables **composition** -- building complex scenes from smaller reusable pieces.

### Manual nesting

Instantiate one prefab and attach children from another:

```cpp
// Load a room prefab
auto room_instance = create_child<PrefabInstance>(
    assets->load_prefab("models/room.glb")
);

// Load an enemy prefab and place it inside the room
auto enemy_prefab = assets->load_prefab("models/enemy.glb", PrefabLoadOptions(), GARBAGE_COLLECT_NEVER);
auto enemy = room_instance->create_child<PrefabInstance>(enemy_prefab);
enemy->transform->set_position(Vec3(5, 0, 3));
```

### Programmatic prefab creation for nesting

You can build a hierarchy of nodes in code and save it as a prefab, then nest it elsewhere:

```cpp
// Build a guard post in code
auto post = scene->create_child<Stage>()->set_name_and_get("GuardPost");
auto guard = post->create_child<Actor>(guard_mesh);
auto torch = post->create_child<Actor>(torch_mesh);

// Capture it as a prefab
auto guard_post_prefab = assets->create_prefab(post);

// Now you can instantiate it anywhere
auto post_1 = scene->create_child<PrefabInstance>(guard_post_prefab);
post_1->transform->set_position(Vec3(0, 0, 0));

auto post_2 = scene->create_child<PrefabInstance>(guard_post_prefab);
post_2->transform->set_position(Vec3(10, 0, 0));
```

### glTF scene nesting

A glTF file can itself reference other glTF files through its scene hierarchy. The GLTF loader processes the full tree, so nested prefabs from a single file load correctly.

---

## 8. Prefab Best Practices for Organization

### 1. Use a consistent folder structure

Organize prefabs by category so they are easy to find:

```
assets/
  prefabs/
    characters/
      hero.glb
      enemy_basic.glb
      enemy_elite.glb
    environments/
      room_01.glb
      dungeon_corridor.glb
    props/
      crate.glb
      barrel.glb
      torch.glb
```

### 2. Name nodes meaningfully

Give every node in your glTF file a clear, descriptive name. This makes `find_descendent_with_name()` calls reliable and debugging easier:

```
Good:    "WeaponMount", "SpawnPoint_01", "LeftHand_IK"
Avoid:   "Cube.042", "Empty.003", "Bone.017"
```

### 3. Keep prefabs focused

A prefab should represent a single logical unit. A "Character" prefab is good. A "WholeLevelWithCharactersAndPropsAndLighting" prefab is too broad -- split it into smaller prefabs and compose them.

### 4. Use GARBAGE_COLLECT_NEVER for frequently spawned prefabs

If you spawn the same prefab many times during gameplay, keep it loaded:

```cpp
// Good for prefabs you spawn repeatedly
PrefabPtr bullet_prefab = assets->load_prefab(
    "models/bullet.glb",
    PrefabLoadOptions(),
    GARBAGE_COLLECT_NEVER
);
```

### 5. Use Stages for logical grouping inside prefabs

When building prefabs in a 3D modelling tool, use empty nodes as grouping containers. These become `Stage` nodes in Simulant and help you organize the hierarchy:

```
Character (Stage)
  |-- Body (Actor)
  |-- Head (Actor)
  |-- Weapons (Stage)
  |     |-- Gun (Actor)
  |     `-- Grenade (Actor)
  `-- Effects (Stage)
        |-- Trail (ParticleSystem)
        `-- Glow (Sprite)
```

### 6. Preview prefabs with the engine tools

Load prefabs into a simple test scene to verify they look correct before integrating them into your game. The sample code in `samples/anim.cpp` is a good starting point for this.

---

## 9. Dynamic Loading and Unloading of Prefabs

Prefabs can be loaded and unloaded at runtime, enabling streaming, level transitions, and memory management.

### Loading on demand

```cpp
void spawn_enemy(const Vec3& position) {
    auto prefab = assets->load_prefab("models/enemy.glb");
    auto instance = scene->create_child<PrefabInstance>(prefab);
    instance->transform->set_position(position);
}
```

### Background loading

Use the scene's background preloading to avoid hitches:

```cpp
scenes->preload_in_background("game_level").then([this]() {
    // Prefabs are now loaded, safe to instantiate
    auto room = create_child<PrefabInstance>(
        assets->load_prefab("models/room.glb")
    );
    scenes->activate("game_level");
});
```

### Unloading prefabs

When you are done with a prefab and want to free its memory:

```cpp
// If you loaded with GARBAGE_COLLECT_NEVER, switch to periodic:
prefab->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);

// Or explicitly destroy the prefab:
assets->destroy_prefab(prefab->id());
```

Destroying the prefab does **not** destroy existing instances. Only future instantiations will fail. Existing `PrefabInstance` nodes in the scene continue to function.

### Destroying prefab instances

To remove a prefab instance and all its nodes from the scene:

```cpp
instance->destroy();
// All child nodes and mixins are queued for destruction automatically
```

### Spawning and despawning pools

For games that need to spawn and despawn objects frequently (enemies, projectiles, pickups), combine prefab loading with a pooling pattern:

```cpp
class EnemyPool {
public:
    EnemyPool(AssetManager* assets, Scene* scene, const Path& prefab_path, int count) {
        prefab_ = assets->load_prefab(prefab_path, PrefabLoadOptions(), GARBAGE_COLLECT_NEVER);
        for (int i = 0; i < count; ++i) {
            auto instance = scene->create_child<PrefabInstance>(prefab_);
            instance->set_visible(false);
            available_.push_back(instance);
        }
    }

    PrefabInstance* spawn(const Vec3& position) {
        if (available_.empty()) return nullptr;
        auto* instance = available_.back();
        available_.pop_back();
        instance->set_visible(true);
        instance->transform->set_position(position);
        return instance;
    }

    void despawn(PrefabInstance* instance) {
        instance->set_visible(false);
        available_.push_back(instance);
    }

private:
    PrefabPtr prefab_;
    std::vector<PrefabInstance*> available_;
};
```

---

## 10. Complete Example: Loading a Character Prefab

This example demonstrates loading a character prefab from a GLB file, setting up animations, positioning the character, and rendering it. It is based on the engine's own `anim.cpp` sample.

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class GameScene : public Scene {
public:
    GameScene(Window* window) : Scene(window) {}

    void on_load() override {
        // ---- Step 1: Load the character prefab ----
        // The GLB file contains meshes, materials, skeleton data, and animations.
        PrefabPtr character_prefab = assets->load_prefab("assets/character.glb");

        // ---- Step 2: Instantiate the prefab ----
        // This creates the full node hierarchy from the GLB file.
        // An AnimationController mixin is automatically added if the file has animations.
        character_ = create_child<PrefabInstance>(character_prefab);

        // ---- Step 3: Access and configure animations ----
        auto anim_controller = character_->find_mixin<AnimationController>();
        if (anim_controller) {
            auto animations = anim_controller->animation_names();

            // Log available animations for debugging
            for (auto& name : animations) {
                S_DEBUG("Found animation: {0}", name);
            }

            if (!animations.empty()) {
                // Play the first animation (e.g., "idle")
                anim_controller->play(animations[0]);

                // Queue subsequent animations to play in sequence
                for (size_t i = 1; i < animations.size() && i < 6; ++i) {
                    anim_controller->queue(animations[i]);
                }
            }
        }

        // ---- Step 4: Access specific nodes within the prefab ----
        // Find a named actor to modify its material
        auto actors = character_->find_descendents_by_types({Actor::Meta::node_type});
        if (!actors.empty()) {
            auto* main_actor = static_cast<Actor*>(actors[0]);
            // Example: disable lighting on the first submesh
            main_actor->base_mesh()->first_submesh()->material()->set_lighting_enabled(false);
        }

        // ---- Step 5: Position the character in the world ----
        character_->transform->set_position(Vec3(0, -1, -5.0f));

        // ---- Step 6: Set up a camera ----
        auto camera = create_child<Camera3D>({
            {"znear",  0.1f},
            {"zfar",   100.0f},
            {"aspect", window->aspect_ratio()},
            {"yfov",   45.0f}
        });

        camera->set_perspective_projection(
            Degrees(45.0),
            window->aspect_ratio(),
            1.0f,
            1000.0f
        );

        // ---- Step 7: Create a render layer ----
        // This connects the character and camera to the rendering pipeline.
        auto layer = compositor->create_layer(character_, camera);
        layer->set_clear_flags(BUFFER_CLEAR_ALL);
        layer->viewport->set_color(Color::gray());
    }

private:
    PrefabInstance* character_ = nullptr;
};

class CharacterDemo : public Application {
public:
    CharacterDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<GameScene>("main");
        scenes->activate("_loading");

        // Preload the scene in the background, then activate when ready
        scenes->preload_in_background("main").then([this]() {
            scenes->activate("main");
        });

        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Character Prefab Demo";
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    CharacterDemo app(config);
    return app.run();
}
```

### What this example does, step by step

1. **`load_prefab("assets/character.glb")`** -- Reads the GLB file and builds a `Prefab` containing all nodes, meshes, materials, textures, and animation data.

2. **`create_child<PrefabInstance>(prefab)`** -- Instantiates the prefab. The GLTF loader creates Actor nodes for meshes, Stage nodes for grouping, and attaches an `AnimationController` mixin. Skinned meshes get their skeletons bound to joint nodes.

3. **`find_mixin<AnimationController>()`** -- Retrieves the animation controller that was automatically created. Lists available animation names and sets up playback.

4. **`find_descendents_by_types(...)`** -- Searches the instantiated hierarchy for Actor nodes, demonstrating how to reach inside the prefab to modify individual components.

5. **`transform->set_position(...)`** -- Moves the entire character (all its child nodes) to a position in the world.

6. **Camera and Layer setup** -- Creates a camera and a compositor layer so the character is actually rendered to the screen.

---

## Summary

| Concept | Key Methods / Types |
|---------|-------------------|
| Loading a prefab | `assets->load_prefab("file.glb")` |
| Creating a prefab from nodes | `assets->create_prefab(root_node)` |
| Instantiating | `create_child<PrefabInstance>(prefab)` |
| Finding nodes | `find_descendent_with_name()`, `find_descendents_by_types()` |
| Animation control | `find_mixin<AnimationController>()` |
| Transform | `instance->transform->set_position(...)`, etc. |
| Garbage collection | `GARBAGE_COLLECT_NEVER` / `GARBAGE_COLLECT_PERIODIC` |
| Destroying a prefab | `assets->destroy_prefab(prefab_id)` |
| Destroying an instance | `instance->destroy()` |
