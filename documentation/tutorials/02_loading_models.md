# Tutorial 2: Loading Models and Working with Prefabs

In this tutorial, you will learn how to load 3D models into your Simulant game using the prefab system. You will load a character model from a GLB file, place it in a scene, and learn how to find and manipulate individual parts of the model.

**Prerequisites:** [Tutorial 1 -- Basic Application](01_basic_application.md)

**Related documentation:** [Prefabs](../assets/prefabs.md), [Mesh Formats](../assets/mesh-formats.md), [Stage Nodes](../core-concepts/stage-nodes.md).

---

## What You Will Build

By the end of this tutorial, you will have a working application that:

- Loads a 3D model from a GLB file
- Instantiates it in the scene
- Finds specific nodes within the model hierarchy
- Positions and orients the model in the world
- Renders it with a camera and lighting

---

## Step 1: Understanding Prefabs

A **Prefab** is a template for a hierarchy of nodes stored in a file. Think of it as a blueprint that describes a tree of nodes -- their types, transforms, meshes, materials, and animations.

A **PrefabInstance** is what actually places that blueprint into your scene. One prefab can be instantiated many times, each producing an independent copy of the node hierarchy.

The primary prefab format in Simulant is **glTF 2.0** (both `.gltf` JSON and `.glb` binary formats). GLB is recommended because it embeds all meshes, textures, and materials in a single self-contained file.

### What happens when you load a GLB file?

The GLTF loader parses the file and builds a `Prefab` internally:

| GLTF Element | Becomes a Prefab... |
|--------------|---------------------|
| Nodes | `PrefabNode` entries with type, name, and transform |
| Meshes | Loaded as `Mesh` assets, referenced by actor nodes |
| Materials | Loaded as `Material` assets |
| Textures | Loaded as `Texture` assets |
| Animations | Animation channels ready for playback |
| Skins | Skeletal data linked to actor meshes |
| Lights (KHR extension) | Light nodes with color, intensity, range |

---

## Step 2: Setting Up the Application

Start with the basic application structure from Tutorial 1:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class ModelScene : public Scene {
public:
    ModelScene(Window* window) : Scene(window) {}

    void on_load() override {
        // We will load our model here
    }
};

class ModelDemo : public Application {
public:
    ModelDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<ModelScene>("main");
        scenes->activate("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Model Loading Demo";
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    ModelDemo app(config);
    return app.run(argc, argv);
}
```

---

## Step 3: Loading a Prefab

Inside the `on_load()` method of your scene, load the prefab from a GLB file:

```cpp
void on_load() override {
    // Load the prefab from a GLB file
    PrefabPtr model_prefab = assets->load_prefab("models/character.glb");

    if (!model_prefab) {
        S_ERROR("Failed to load prefab!");
        return;
    }

    S_DEBUG("Prefab loaded successfully");
}
```

The `load_prefab()` method searches for the file relative to your asset paths. If the file is not found, it returns a null pointer.

### Keeping frequently used prefabs in memory

If you plan to spawn the same prefab many times (e.g., enemies, bullets), keep it loaded by passing `GARBAGE_COLLECT_NEVER`:

```cpp
PrefabPtr enemy_prefab = assets->load_prefab(
    "models/enemy.glb",
    PrefabLoadOptions(),
    GARBAGE_COLLECT_NEVER
);

// Later, when completely done with it:
enemy_prefab->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);
```

---

## Step 4: Instantiating the Prefab

Loading the prefab only reads it from disk. To actually place it in your scene, create a `PrefabInstance`:

```cpp
void on_load() override {
    PrefabPtr model_prefab = assets->load_prefab("models/character.glb");

    // Instantiate the prefab -- this creates all the nodes in the scene
    model_instance_ = create_child<PrefabInstance>(model_prefab);

    // The PrefabInstance acts as the root of the spawned hierarchy
    // Move the entire model by moving the instance
    model_instance_->transform->set_position(Vec3(0, 0, -5.0f));
}
```

That is all it takes. The `PrefabInstance`:

1. Reads every node from the prefab
2. Creates the corresponding `StageNode` objects (Actors, Cameras, Lights, Stages)
3. Sets up parent-child relationships to match the original hierarchy
4. If the prefab has animations, attaches an `AnimationController` mixin
5. If the prefab has skinned meshes, binds the skeleton to the joint nodes

---

## Step 5: Setting Up a Camera

To actually see the model, you need a camera and a render layer:

```cpp
void on_load() override {
    // Load and instantiate the model
    PrefabPtr model_prefab = assets->load_prefab("models/character.glb");
    model_instance_ = create_child<PrefabInstance>(model_prefab);
    model_instance_->transform->set_position(Vec3(0, 0, -5.0f));

    // Create a 3D camera
    auto camera = create_child<Camera3D>({
        {"znear",  0.1f},
        {"zfar",   100.0f},
        {"aspect", window->aspect_ratio()},
        {"yfov",   45.0f}
    });

    camera->set_perspective_projection(
        Degrees(45.0),
        window->aspect_ratio(),
        0.1f,
        100.0f
    );

    camera->transform->set_position(Vec3(0, 2, 3));
    camera->transform->look_at(Vec3(0, 0, -5));

    // Create a render layer to connect the model and camera
    auto layer = compositor->create_layer(model_instance_, camera);
    layer->set_clear_flags(BUFFER_CLEAR_ALL);
    layer->viewport->set_color(Color::gray());
}
```

---

## Step 6: Finding Nodes Inside a Prefab

Once a prefab is instantiated, you often need to reach into the hierarchy to access specific nodes. For example, to attach a weapon to a character's hand, or to read a spawn point's position.

### Finding by name

```cpp
// Find a specific node by its name
StageNode* weapon_mount = model_instance_->find_descendent_with_name("WeaponMount");
if (weapon_mount) {
    S_DEBUG("Found weapon mount!");
    // You can attach things to this node
    auto gun = weapon_mount->create_child<Actor>(gun_mesh);
}
```

### Finding by type

Search for all nodes of a particular type:

```cpp
// Find all Actor nodes in the prefab instance
std::vector<StageNode*> actors =
    model_instance_->find_descendents_by_types({Actor::Meta::node_type});

for (auto* node : actors) {
    auto* actor = static_cast<Actor*>(node);
    S_DEBUG("Actor: {}", actor->name().c_str());
}
```

### Traversing the hierarchy

You can iterate over children and descendents:

```cpp
// Iterate immediate children
for (auto& child : model_instance_->each_child()) {
    S_DEBUG("Child: {}", child.name().c_str());
}

// Iterate all descendents
for (auto& desc : model_instance_->each_descendent()) {
    S_DEBUG("Node: {} (type: {})", desc.name().c_str(), desc.node_type_name().c_str());
}
```

---

## Step 7: Modifying the Model

Each `PrefabInstance` is an independent copy. Changes you make to the instance do **not** affect the original prefab or other instances.

### Transform changes

```cpp
// Position the instance
model_instance_->transform->set_position(Vec3(10, 0, 5));

// Rotate the entire instance
model_instance_->transform->set_rotation(
    Quaternion::from_axis_angle(Vec3::UP, Degrees(90))
);

// Scale the entire instance
model_instance_->transform->set_scale_factor(Vec3(2.0f, 2.0f, 2.0f));
```

### Modifying materials on specific parts

```cpp
// Find a specific actor and change its material
auto actors = model_instance_->find_descendents_by_types({Actor::Meta::node_type});
for (auto* node : actors) {
    auto* actor = static_cast<Actor*>(node);
    if (actor->name() == "Body") {
        // Disable lighting on this part
        actor->base_mesh()->first_submesh()->material()->set_lighting_enabled(false);
    }
}
```

---

## Step 8: Creating Prefabs from Code

You can also create prefabs **programmatically** from nodes already in your scene. This is useful for saving player-built structures or snapshotting a layout:

```cpp
// Build something in code
auto group = scene->create_child<Stage>()->set_name_and_get("MyStructure");
auto box = group->create_child<Actor>(assets->new_mesh_from_procedural_cube());
box->scale_by(1, 1, 1);

// Capture it as a prefab
auto saved_prefab = assets->create_prefab(group);

// Now you can instantiate it elsewhere
auto clone = scene->create_child<PrefabInstance>(saved_prefab);
clone->transform->set_position(Vec3(5, 0, 0));
```

---

## Step 9: Nested Prefabs

A prefab can contain any type of StageNode, including other prefabs. This enables **composition** -- building complex scenes from smaller reusable pieces.

```cpp
// Load a room prefab
auto room = create_child<PrefabInstance>(
    assets->load_prefab("models/room.glb")
);

// Load an enemy prefab and place it inside the room
PrefabPtr enemy_prefab = assets->load_prefab(
    "models/enemy.glb",
    PrefabLoadOptions(),
    GARBAGE_COLLECT_NEVER
);
auto enemy = room->create_child<PrefabInstance>(enemy_prefab);
enemy->transform->set_position(Vec3(5, 0, 3));
```

---

## Complete Example

Here is the full working application that loads a model and renders it:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class ModelScene : public Scene {
public:
    ModelScene(Window* window) : Scene(window) {}

    void on_load() override {
        // ---- Step 1: Load the prefab ----
        PrefabPtr model_prefab = assets->load_prefab("models/character.glb");

        if (!model_prefab) {
            S_ERROR("Failed to load model!");
            return;
        }

        // ---- Step 2: Instantiate it ----
        model_instance_ = create_child<PrefabInstance>(model_prefab);

        // ---- Step 3: Position it in the world ----
        model_instance_->transform->set_position(Vec3(0, 0, -5.0f));

        // ---- Step 4: Find and log all actors ----
        auto actors = model_instance_->find_descendents_by_types(
            {Actor::Meta::node_type}
        );
        S_DEBUG("Found {} actors in model:", actors.size());
        for (auto* node : actors) {
            auto* actor = static_cast<Actor*>(node);
            S_DEBUG("  - {}", actor->name().c_str());
        }

        // ---- Step 5: Set up a camera ----
        auto camera = create_child<Camera3D>({
            {"znear",  0.1f},
            {"zfar",   100.0f},
            {"aspect", window->aspect_ratio()},
            {"yfov",   45.0f}
        });

        camera->set_perspective_projection(
            Degrees(45.0),
            window->aspect_ratio(),
            0.1f,
            100.0f
        );

        camera->transform->set_position(Vec3(0, 2, 3));
        camera->transform->look_at(Vec3(0, 0, -5));

        // ---- Step 6: Create a render layer ----
        auto layer = compositor->create_layer(model_instance_, camera);
        layer->set_clear_flags(BUFFER_CLEAR_ALL);
        layer->viewport->set_color(Color::gray());
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        // Slowly rotate the model
        if (model_instance_) {
            float angle = Degrees(45).radians() * dt;
            auto current = model_instance_->transform->rotation();
            auto rotated = Quaternion::angle_axis(Radians(angle), Vec3::UP) * current;
            model_instance_->transform->set_rotation(rotated);
        }
    }

private:
    PrefabInstance* model_instance_ = nullptr;
};

class ModelDemo : public Application {
public:
    ModelDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<ModelScene>("main");
        scenes->activate("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Model Loading Demo";
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    ModelDemo app(config);
    return app.run(argc, argv);
}
```

---

## Best Practices for Organizing Models

### 1. Use a consistent folder structure

```
assets/
  models/
    characters/
      hero.glb
      enemy_basic.glb
    environments/
      room_01.glb
    props/
      crate.glb
      barrel.glb
```

### 2. Name nodes meaningfully in your 3D modelling tool

Give every node a clear, descriptive name. This makes `find_descendent_with_name()` calls reliable:

```
Good:    "WeaponMount", "SpawnPoint_01", "LeftHand"
Avoid:   "Cube.042", "Empty.003", "Bone.017"
```

### 3. Export glTF as Y-up

Simulant expects glTF files to be exported as Y-up. Make sure your 3D modelling tool is configured for Y-up export.

### 4. Keep prefabs focused

A prefab should represent a single logical unit. A "Character" prefab is good. A "WholeLevel" prefab is too broad -- split it into smaller prefabs and compose them.

---

## Summary

| Concept | Key Methods / Types |
|---------|-------------------|
| Loading a prefab | `assets->load_prefab("file.glb")` |
| Creating a prefab from code | `assets->create_prefab(root_node)` |
| Instantiating | `create_child<PrefabInstance>(prefab)` |
| Finding nodes | `find_descendent_with_name()`, `find_descendents_by_types()` |
| Transform | `instance->transform->set_position(...)`, etc. |
| Garbage collection | `GARBAGE_COLLECT_NEVER` / `GARBAGE_COLLECT_PERIODIC` |
| Destroying a prefab | `assets->destroy_prefab(prefab_id)` |
| Destroying an instance | `instance->destroy()` |

**Next:** [Tutorial 3 -- Physics Basics](03_physics.md)
