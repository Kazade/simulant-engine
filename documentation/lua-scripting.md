# Lua Scripting

Simulant provides a Lua scripting system that allows you to define custom `StageNode` behaviour classes in Lua scripts. This is ideal for rapid prototyping, mod support, and gameplay logic that benefits from hot-reloading without recompiling C++ code.

**Related documentation:** [Stage Nodes](../core-concepts/stage-nodes.md), [Behaviours](../scripting/behaviours.md), [Asset Managers](../assets/asset-managers.md), [Coroutines](../coroutines.md).

---

## Table of Contents

1. [Overview](#1-overview)
2. [Defining a Node Class](#2-defining-a-node-class)
3. [Registering and Spawning](#3-registering-and-spawning)
4. [Lifecycle Methods](#4-lifecycle-methods)
5. [Parameters](#5-parameters)
6. [Accessing Assets](#6-accessing-assets)
7. [Creating Children and Mixins from Lua](#7-creating-children-and-mixins-from-lua)
8. [Transform Access](#8-transform-access)
9. [Math Types](#9-math-types)
10. [Loading Scripts from Files](#10-loading-scripts-from-files)
11. [Limitations and Best Practices](#11-limitations-and-best-practices)

---

## 1. Overview

The Lua scripting system lets you:

- **Define StageNode classes** in Lua that behave identically to C++ StageNode subclasses.
- **Implement lifecycle callbacks** (`on_create`, `on_update`, `on_destroy`, etc.) in Lua.
- **Declare parameters** with types, defaults, and descriptions.
- **Access assets**, transforms, and the scene from Lua.
- **Create child nodes and mixins** dynamically from within scripts.

Scripts are loaded and registered through the scene's `register_stage_node` API. Once registered, Lua nodes can be spawned using `scene->create_child("node_type_name")` just like C++ nodes.

---

## 2. Defining a Node Class

Use `smlt.define_node(name)` to create a new StageNode class:

```lua
-- Define a rotating cube behaviour
RotatingCube = smlt.define_node("rotating_cube")

function RotatingCube:on_create(params)
    -- Called when the node is first created.
    -- Return false to abort creation.
    self.speed = 90  -- degrees per second
    return true
end

function RotatingCube:on_update(dt)
    -- Called every frame with variable timestep.
    self.transform:rotate_2d(smlt.Degrees(self.speed * dt))
end
```

`define_node` returns a class table that you assign methods to using the standard Lua `:` syntax. The class is automatically set up with:

- An `__index` metamethod so that `self.transform`, `self.scene`, etc. forward to the underlying C++ node.
- A `Meta` table containing the node type hash and name.
- A `:new(scene)` constructor used internally by the engine.

---

## 3. Registering and Spawning

### Registering from a String

```cpp
const char* script = R"(
BallNode = smlt.define_node("ball_node")

function BallNode:on_update(dt)
    print("Ball update, dt = " .. dt)
end
)";

scene->register_stage_node(script, "BallNode");
```

### Registering from a File

```cpp
scene->register_stage_node(Path("scripts/ball.lua"), "BallNode");
```

### Spawning

Once registered, spawn the node like any other StageNode:

```cpp
// Spawn with no parameters
auto ball = scene->create_child("ball_node");

// Spawn with parameters (see Parameters section)
auto ball2 = scene->create_child("ball_node", {{"speed", 150.0f}});
```

You can also spawn from within Lua (see [Creating Children and Mixins](#7-creating-children-and-mixins-from-lua)).

---

## 4. Lifecycle Methods

Lua StageNode classes can implement any of the following lifecycle methods. All are optional -- if not defined, the default C++ behaviour applies.

| Method | Signature | Description |
|--------|-----------|-------------|
| `on_create` | `function on_create(params)` | Called when the node is created. Receives a table of parameters. Return `false` to abort creation. |
| `on_update` | `function on_update(dt)` | Called every frame. `dt` is the variable timestep in seconds. |
| `on_fixed_update` | `function on_fixed_update(step)` | Called at a fixed timestep (typically 1/60s). Use for physics logic. |
| `on_late_update` | `function on_late_update(dt)` | Called after all `on_update` calls. Use for camera follow, animation blending. |
| `on_destroy` | `function on_destroy()` | Called when `destroy()` is invoked. Return `false` to cancel destruction. |
| `on_clean_up` | `function on_clean_up()` | Called just before the node is actually deleted. Use for final resource release. |
| `on_parent_set` | `function on_parent_set()` | Called when the node's parent changes. |
| `on_transformation_changed` | `function on_transformation_changed()` | Called when the node's transform is modified. |

### Example: Full Lifecycle

```lua
LifecycleNode = smlt.define_node("lifecycle_node")

function LifecycleNode:on_create(params)
    print("Node created: " .. self.name)
    self.tick_count = 0
    return true
end

function LifecycleNode:on_update(dt)
    self.tick_count = self.tick_count + 1
    if self.tick_count % 60 == 0 then
        print("One second has passed")
    end
end

function LifecycleNode:on_destroy()
    print("Node " .. self.name .. " is being destroyed")
    return true  -- Allow destruction
end

function LifecycleNode:on_clean_up()
    print("Node " .. self.name .. " cleaned up")
end
```

### on_create Parameters

The `params` argument to `on_create` is a Lua table containing the values passed from C++ (with defaults applied for optional parameters). See the [Parameters](#5-parameters) section for details.

---

## 5. Parameters

### Defining Parameters

Use `smlt.define_node_param(type, description, default_value)` to declare parameters on your node class:

```lua
ParamNode = smlt.define_node("param_node")
ParamNode.params = {
    speed = smlt.define_node_param(smlt.NodeParamType.Float, "Movement speed", 100.0),
    damage = smlt.define_node_param(smlt.NodeParamType.Int, "Damage per hit", 10),
    is_hostile = smlt.define_node_param(smlt.NodeParamType.Bool, "Is hostile?", true),
    label = smlt.define_node_param(smlt.NodeParamType.String, "Display label", "enemy"),
    waypoints = smlt.define_node_param(smlt.NodeParamType.FloatArray, "Waypoint distances")
}

function ParamNode:on_create(params)
    -- params is a table with keys matching the declared params:
    -- params.speed, params.damage, params.is_hostile, params.label, params.waypoints
    
    -- Required params with no default that weren't provided will be nil.
    if params.waypoints == nil then
        print("No waypoints provided, using defaults")
    end
    
    return true
end
```

### Parameter Types

| Lua Constant | C++ Type | Notes |
|-------------|----------|-------|
| `smlt.NodeParamType.Float` | `float` | |
| `smlt.NodeParamType.FloatArray` | `FloatArray` (`std::vector<float>`) | |
| `smlt.NodeParamType.Int` | `int` | |
| `smlt.NodeParamType.IntArray` | `IntArray` | |
| `smlt.NodeParamType.Bool` | `bool` | |
| `smlt.NodeParamType.BoolArray` | `BoolArray` | |
| `smlt.NodeParamType.String` | `std::string` | |
| `smlt.NodeParamType.MeshPtr` | `MeshPtr` | Not converted in `on_create` params table |
| `smlt.NodeParamType.TexturePtr` | `TexturePtr` | Not converted in `on_create` params table |
| `smlt.NodeParamType.ParticleScriptPtr` | `ParticleScriptPtr` | Not converted in `on_create` params table |
| `smlt.NodeParamType.StageNodePtr` | `StageNode*` | Not converted in `on_create` params table |
| `smlt.NodeParamType.PrefabPtr` | `PrefabPtr` | Not converted in `on_create` params table |

### Passing Parameters from C++

```cpp
auto node = scene->create_child("param_node", {
    {"speed", 200.0f},
    {"damage", 25},
    {"is_hostile", true}
});
```

Parameters not provided will use their declared defaults. Required parameters (no default) that are omitted will cause creation to fail (`on_create` returns `false`).

---

## 6. Accessing Assets

Every Lua StageNode has access to the scene's `AssetManager` through `self.assets`. This allows scripts to load meshes, textures, materials, sounds, fonts, and prefabs at runtime.

### Available Asset Methods

```lua
function AssetNode:on_create(params)
    -- Access the asset manager
    local assets = self.assets
    assert(assets ~= nil, "self.assets should not be nil")
    
    -- Load a mesh
    local mesh = assets:mesh("assets/samples/cube.obj")
    
    -- Load a texture
    local tex = assets:texture("assets/textures/brick.png")
    
    -- Load a material
    local mat = assets:material("materials/my_material.smat")
    
    -- Load a sound
    local sound = assets:sound("assets/sounds/explosion.ogg")
    
    -- Load a font
    local font = assets:font("assets/fonts/Orbitron-Regular.ttf")
    
    -- Load a prefab
    local prefab = assets:prefab("assets/prefabs/tree.prefab")
    
    -- Check if an asset is loaded
    if assets:has_mesh("assets/samples/cube.obj") then
        print("Cube mesh is already loaded")
    end
    
    return true
end
```

### Checking Asset Availability

The `AssetManager` also provides count methods for debugging and introspection:

```lua
print("Loaded meshes: " .. assets:mesh_count())
print("Loaded textures: " .. assets:texture_count())
print("Loaded materials: " .. assets:material_count())
print("Loaded sounds: " .. assets:sound_count())
print("Loaded fonts: " .. assets:font_count())
print("Loaded prefabs: " .. assets:prefab_count())
```

---

## 7. Creating Children and Mixins from Lua

### Creating Child Nodes

Use `smlt.create_child_node(parent, type_name, params)` to create a child from within a Lua script:

```lua
ParentNode = smlt.define_node("parent_node")

function ParentNode:on_create(params)
    self.transform.translation = smlt.Vec3(100, 0, 0)
    
    -- Create a child using the type name and optional params table
    local child = smlt.create_child_node(self, "child_type", {
        x_pos = 42.0,
        y_pos = 99.0
    })
    
    if child then
        print("Child created successfully")
    end
    
    return true
end
```

### Creating Mixins

Use `smlt.create_mixin(host, type_name, params)` to attach a mixin to the current node:

```lua
MixinHost = smlt.define_node("mixin_host")

function MixinHost:on_create(params)
    self.transform.translation = smlt.Vec3(5, 10, 15)
    
    -- Create a mixin (shares the host's transform)
    local mixin = smlt.create_mixin(self, "mixin_type", {
        scale_factor = 3.0
    })
    
    if not mixin then
        print("ERROR: create_mixin returned nil")
        return false
    end
    
    -- The mixin shares the same transform as the host
    local mixin_pos = mixin.transform.position
    assert(math.abs(mixin_pos.x - 5) < 0.01, "Mixin transform should match host")
    
    self.my_mixin = mixin
    return true
end
```

---

## 8. Transform Access

Every Lua StageNode has access to its transform through `self.transform`. The transform is the same C++ object as the underlying StageNode's transform -- writes from Lua are visible from C++ and vice versa.

### Properties

```lua
-- World-space (absolute) properties
local pos = self.transform.position        -- Vec3, read/write
local ori = self.transform.orientation     -- Quaternion, read/write
local scl = self.transform.scale           -- Vec3, read/write

-- Parent-relative (local) properties
local tr = self.transform.translation      -- Vec3, read/write
local rot = self.transform.rotation        -- Quaternion, read/write
local sf = self.transform.scale_factor     -- Vec3, read/write
```

### Methods

```lua
-- Mutation helpers
self.transform:translate(smlt.Vec3(1, 0, 0))
self.transform:rotate(smlt.Quaternion(0, 0, 0, 1))
self.transform:rotate(axis_vec3, smlt.Degrees(45))
self.transform:rotate(pitch_deg, yaw_deg, roll_deg)
self.transform:scale_by(smlt.Vec3(2, 2, 2))
self.transform:scale_by(2.0)
self.transform:look_at(target_vec3)
self.transform:look_at(target_vec3, up_vec3)

-- Direction helpers (read-only, derived from world orientation)
local fwd = self.transform:forward()
local up = self.transform:up()
local right = self.transform:right()
```

### 2D Helpers

For 2D scenes, use the 2D-specific properties:

```lua
self.transform.position_2d = smlt.Vec2(100, 200)
self.transform.translation_2d = smlt.Vec2(10, 0)
self.transform.orientation_2d = smlt.Degrees(90)
self.transform.rotation_2d = smlt.Degrees(45)
self.transform:set_scale_factor_2d(smlt.Vec2(2, 2))
self.transform:translate_2d(smlt.Vec2(5, 0))
self.transform:rotate_2d(smlt.Degrees(15))
```

---

## 9. Math Types

Simulant exposes the following math types to Lua:

### Vec2

```lua
local v = smlt.Vec2(x, y)     -- Explicit components
local v = smlt.Vec2()          -- Zero vector (0, 0)

print(v.x, v.y)                -- Read/write properties
v.x = 10

local len = v:length()         -- Length
local len2 = v:length_squared()-- Squared length
v:normalize()                  -- Normalize in place
local n = v:normalized()       -- Returns normalized copy
local d = v:dot(other)         -- Dot product
local l = v:lerp(other, 0.5)   -- Linear interpolation
```

### Vec3

```lua
local v = smlt.Vec3(x, y, z)   -- Explicit components
local v = smlt.Vec3(s)         -- Fill all components (s, s, s)
local v = smlt.Vec3()          -- Zero vector (0, 0, 0)

print(v.x, v.y, v.z)           -- Read/write properties

local len = v:length()
local len2 = v:length_squared()
v:normalize()
local n = v:normalized()
local d = v:dot(other)
local c = v:cross(other)       -- Cross product
local l = v:lerp(other, 0.5)
local dist = v:distance_to(other)

-- Static constants
smlt.Vec3.up         -- (0, 1, 0)
smlt.Vec3.down       -- (0, -1, 0)
smlt.Vec3.left       -- (-1, 0, 0)
smlt.Vec3.right      -- (1, 0, 0)
smlt.Vec3.forward    -- (0, 0, 1)
smlt.Vec3.backward   -- (0, 0, -1)
smlt.Vec3.zero       -- (0, 0, 0)
smlt.Vec3.one        -- (1, 1, 1)
```

### Quaternion

```lua
local q = smlt.Quaternion()                                  -- Identity
local q = smlt.Quaternion(x, y, z, w)                       -- Explicit xyzw
local q = smlt.Quaternion(axis_vec3, smlt.Degrees(90))      -- Axis-angle
local q = smlt.Quaternion(pitch, yaw, roll)                 -- Euler angles (Degrees)

print(q.x, q.y, q.z, q.w)    -- Read/write properties
local n = q:normalized()     -- Returns normalized copy
local fwd = q:forward()      -- Forward direction
local up = q:up()            -- Up direction
local right = q:right()      -- Right direction
local s = q:slerp(other, 0.5)-- Spherical interpolation
local n2 = q:nlerp(other, 0.5)-- Normalized linear interpolation
```

### Degrees

```lua
local d = smlt.Degrees(90)
local val = d:to_float()    -- Returns 90.0
```

---

## 10. Loading Scripts from Files

### Single File, Multiple Classes

A single `.lua` file can define multiple node classes. Register each one by calling `register_stage_node` with the same file path but different class names:

```cpp
Path script("scripts/behaviours.lua");
scene->register_stage_node(script, "PlayerController");
scene->register_stage_node(script, "EnemyAI");
scene->register_stage_node(script, "PickupBehaviour");
```

Each registration re-loads and executes the file, redefining all classes in the Lua global namespace. Existing spawned instances of previously registered classes remain functional.

### Hot-Reloading

To update a script at runtime, rewrite the file and re-register:

```cpp
// Initial registration
scene->register_stage_node(Path("scripts/player.lua"), "PlayerController");

// ... later, after modifying the file:
scene->register_stage_node(Path("scripts/player.lua"), "PlayerController");
```

**Note:** Re-registering replaces the Lua class definition in the global namespace. Existing C++ StageNode instances remain alive but their Lua wrapper's `__index` metamethod will now resolve methods from the new class definition. Newly spawned instances will use the updated `on_create` logic.

### Error Handling

- If the file does not exist, registration returns `false`.
- If the file contains syntax errors, registration returns `false` and logs the error.
- If the specified class name is not found in the script, registration returns `false`.

---

## 11. Limitations and Best Practices

### Current Limitations

| Limitation | Details |
|------------|---------|
| Complex param types | `MeshPtr`, `TexturePtr`, `PrefabPtr` and similar complex types are not converted in the `on_create` params table. Pass them as asset IDs (strings) and load via `self.assets` inside `on_create`. |
| `create_child_node` params | The `params` table passed to `smlt.create_child_node` supports basic types (float, int, bool, string). Complex asset references must be passed as strings and resolved by the child. |
| Class-level state | Class-level variables (e.g., `MyNode._counter = 0`) are shared across all instances of that class. Be aware that re-registering a script resets these values. |
| No direct C++ method calls from Lua | You cannot call arbitrary C++ methods on `_cpp_node` from Lua due to LuaBridge limitations with the Params binding. Use the exposed properties (`transform`, `scene`, `assets`) and helper functions (`smlt.create_child_node`, `smlt.create_mixin`). |

### Best Practices

1. **Keep scripts focused.** One behaviour per class, just like C++ StageNode design.
2. **Use parameters for configuration.** Declare parameters with `smlt.define_node_param` so they can be set from C++ or prefab files.
3. **Return `false` from `on_create` on failure.** This signals to the engine that the node should not be spawned.
4. **Use `self.assets` for asset loading.** Never hardcode asset paths -- load assets through the asset manager.
5. **Avoid storing Lua references long-term.** The `_cpp_node` forwarding mechanism handles access to the C++ side; don't store references to Lua wrapper tables.
6. **Test parameter defaults.** Missing required parameters will cause creation to fail silently -- always test with and without parameters.

### Complete Example

```lua
-- scripts/enemy.lua

Enemy = smlt.define_node("enemy")
Enemy.params = {
    speed = smlt.define_node_param(smlt.NodeParamType.Float, "Movement speed", 50.0),
    health = smlt.define_node_param(smlt.NodeParamType.Int, "Starting health", 100),
    mesh_id = smlt.define_node_param(smlt.NodeParamType.String, "Mesh asset path")
}

function Enemy:on_create(params)
    self.speed = params.speed
    self.health = params.health
    self.alive = true
    
    -- Load mesh from asset manager
    if params.mesh_id then
        self.mesh_id = params.mesh_id
    end
    
    print("Enemy created with health=" .. self.health)
    return true
end

function Enemy:on_update(dt)
    if not self.alive then return end
    
    -- Move forward
    local fwd = self.transform:forward()
    self.transform:translate(fwd * self.speed * dt)
end

function Enemy:on_destroy()
    print("Enemy destroyed")
    return true
end

function Enemy:take_damage(amount)
    self.health = self.health - amount
    if self.health <= 0 then
        self.alive = false
        self.base():destroy()
    end
end
```

Registration and usage from C++:

```cpp
scene->register_stage_node(Path("scripts/enemy.lua"), "Enemy");

auto enemy = scene->create_child("enemy", {
    {"speed", 75.0f},
    {"health", 150},
    {"mesh_id", "assets/models/enemy.obj"}
});
```
