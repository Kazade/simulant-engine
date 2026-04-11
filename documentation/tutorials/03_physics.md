# Tutorial 3: Physics Basics

In this tutorial, you will learn how to add realistic physics to your Simulant game. You will create a physics scene, add static and dynamic bodies, and watch objects fall, collide, and bounce.

**Prerequisites:** [Tutorial 1 -- Basic Application](01_basic_application.md), [Tutorial 2 -- Loading Models](02_loading_models.md)

**Related documentation:** [Physics Overview](../physics/overview.md), [Rigid Bodies](../physics/rigid-bodies.md), [Colliders](../physics/colliders.md).

---

## What You Will Build

By the end of this tutorial, you will have a working application that:

- Creates a physics simulation with gravity
- Places a static ground plane that objects collide with
- Spawns dynamic boxes that fall and bounce
- Detects and responds to collisions
- Lets you interact with the physics world

---

## Step 1: Understanding Physics in Simulant

Simulant includes a rigid body physics system built on the **Bounce** physics library. Physics is implemented using **Behaviours** -- reusable components that attach to `StageNodes`:

```
StageNode (visual representation)
  └── Physics Behaviour (physical properties)
      ├── DynamicBody  -- Full simulation (falls, bounces, collides)
      ├── StaticBody   -- Immovable obstacle (ground, walls)
      ├── KinematicBody -- Script-controlled (moving platforms)
      └── ReactiveBody -- Responds to forces
```

### The key idea

Instead of manually moving objects every frame, you:

1. Create physics bodies with properties (mass, shape, etc.)
2. Apply forces (gravity, impulses, etc.)
3. Let the physics engine calculate movement and collisions
4. Read the results to update visual positions (this happens automatically)

---

## Step 2: Setting Up the Application

Start with the basic application structure:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class PhysicsScene : public Scene {
public:
    PhysicsScene(Window* window) : Scene(window) {}

    void on_load() override {
        // We will set up physics here
    }
};

class PhysicsDemo : public Application {
public:
    PhysicsDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<PhysicsScene>("main");
        scenes->activate("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Physics Demo";
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    PhysicsDemo app(config);
    return app.run(argc, argv);
}
```

---

## Step 3: Creating a Physics Scene

The easiest way to get started is using the built-in physics helpers. Add this to your `on_load()`:

```cpp
void on_load() override {
    // Set up the camera first
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

    camera->transform->set_position(Vec3(0, 5, 15));
    camera->transform->look_at(Vec3(0, 1, 0));

    // Create the ground
    create_ground();

    // Create falling boxes
    create_falling_boxes();

    // Create a render layer
    auto layer = compositor->create_layer(this, camera);
    layer->set_clear_flags(BUFFER_CLEAR_ALL);
    layer->viewport->set_color(Color(0.2f, 0.2f, 0.3f));
}
```

---

## Step 4: Creating a Static Ground Plane

Static bodies are immovable colliders -- perfect for ground, walls, and obstacles:

```cpp
void create_ground() {
    // Create the visual
    auto ground = create_child<Actor>();
    auto ground_mesh = assets->new_mesh_from_procedural_cube();
    ground->set_mesh(ground_mesh->id());
    ground->scale_by(20, 0.5f, 20);  // Wide, thin, long
    ground->move_to(0, -1, 0);

    // Add a static physics body
    auto static_body = ground->create_child<StaticBody>();
    static_body->add_box_collider(Vec3(20, 0.5f, 20), PhysicsMaterial::stone());

    S_DEBUG("Ground plane created");
}
```

Key points:
- The visual (`Actor`) and the physics body (`StaticBody`) are linked as parent-child
- `add_box_collider()` takes **half-extents** (size from center to edge on each axis)
- `PhysicsMaterial::stone()` gives the ground high friction and low bounciness

---

## Step 5: Creating Dynamic Objects

Dynamic bodies are fully simulated -- they fall, bounce, and collide:

```cpp
void create_falling_boxes() {
    for (int i = 0; i < 5; ++i) {
        // Create the visual
        auto box = create_child<Actor>();
        auto mesh = assets->new_mesh_from_procedural_cube();
        box->set_mesh(mesh->id());

        // Position boxes at different heights
        float x_pos = (i - 2) * 3.0f;  // Spread them out
        box->move_to(x_pos, 10 + i * 2, 0);

        // Add dynamic physics body
        auto body = box->create_child<DynamicBody>();
        body->add_box_collider(Vec3(0.5f, 0.5f, 0.5f), PhysicsMaterial::wood());
        body->set_mass(1.0f);
        body->set_restitution(0.5f);  // Bounciness (0 = dead, 1 = superball)

        S_DEBUG("Box {} created at position ({}, {}, {})",
            i, x_pos, 10.0f + i * 2, 0);
    }
}
```

When you run this, you will see five boxes fall from the sky and bounce on the ground.

### Dynamic body properties you can tweak

```cpp
auto body = box->create_child<DynamicBody>();

// Physical properties
body->set_mass(2.0f);             // Heavier objects are harder to push
body->set_restitution(0.8f);      // Bounciness
body->set_friction(0.5f);         // Surface friction
body->set_linear_damping(0.1f);   // Air resistance
body->set_angular_damping(0.1f);  // Rotational damping

// Lock rotation on specific axes (useful for 2D-style gameplay)
body->lock_rotation(true, false, true);  // Lock X and Z rotation
```

---

## Step 6: Detecting Collisions

You can react to collisions using signals. Let us modify one of the boxes to log when it hits the ground:

```cpp
class BouncingBox : public StageNode {
public:
    FindResult<DynamicBody> body = FindDescendent<DynamicBody>(this);

    void on_load() override {
        // Listen for collisions
        body->signal_collision_enter().connect(
            [this](const Collision& collision) {
                S_INFO("Box hit: {}", collision.other_collider_name);
            }
        );

        body->signal_collision_exit().connect(
            [this](const Collision& collision) {
                S_INFO("Box stopped hitting: {}", collision.other_collider_name);
            }
        );
    }
};
```

To use this custom behaviour, create the box with the behaviour attached:

```cpp
void create_falling_boxes() {
    for (int i = 0; i < 5; ++i) {
        // Create a container node with our custom behaviour
        auto container = create_child<BouncingBox>();

        auto box = container->create_child<Actor>();
        auto mesh = assets->new_mesh_from_procedural_cube();
        box->set_mesh(mesh->id());

        float x_pos = (i - 2) * 3.0f;
        container->move_to(x_pos, 10 + i * 2, 0);

        auto body = box->create_child<DynamicBody>();
        body->add_box_collider(Vec3(0.5f, 0.5f, 0.5f), PhysicsMaterial::wood());
        body->set_mass(1.0f);
        body->set_restitution(0.5f);
    }
}
```

---

## Step 7: Applying Forces and Impulses

You can push objects around using forces and impulses:

```cpp
// Continuous force (like wind or thrust)
body->add_force(Vec3(0, 100, 0));

// Force at a specific point (causes rotation)
body->add_force_at_position(Vec3(0, 100, 0), Vec3(0.5f, 0, 0));

// Impulse -- instant velocity change (like an explosion or kick)
body->add_impulse(Vec3(0, 200, 0));

// Impulse at a specific point
body->add_impulse_at_position(Vec3(0, 200, 0), Vec3(0.5f, 0, 0));

// Torque -- rotational force
body->add_torque(Vec3(0, 10, 0));
```

### Example: Explosive force

Add this to your scene to blast all boxes away:

```cpp
void explosion(Vec3 center, float radius, float force) {
    auto all_boxes = find_descendents_by_types({DynamicBody::Meta::node_type});

    for (auto* node : all_boxes) {
        auto* body = static_cast<DynamicBody*>(node);
        Vec3 body_pos = body->transform->position();
        float dist = (body_pos - center).length();

        if (dist < radius) {
            Vec3 dir = (body_pos - center).normalized();
            float strength = force * (1.0f - dist / radius);
            body->add_impulse(dir * strength);
        }
    }
}

void on_update(float dt) override {
    Scene::on_update(dt);

    // Trigger an explosion when the player presses Space
    auto input = window->input;
    if (input->is_button_down(BUTTON_SPACE)) {
        explosion(Vec3(0, 0, 0), 15.0f, 500.0f);
    }
}
```

---

## Step 8: Creating a Kinematic Body (Moving Platform)

Kinematic bodies are controlled entirely by your code, but they will push dynamic bodies out of their way. They are ideal for moving platforms, elevators, and doors.

```cpp
class MovingPlatform : public StageNode {
public:
    FindResult<KinematicBody> body = FindDescendent<KinematicBody>(this);

    Vec3 start_pos = Vec3(-5, 0, 0);
    Vec3 end_pos = Vec3(5, 0, 0);
    float t = 0.0f;
    bool forward = true;

    void on_load() override {
        body->add_box_collider(
            Vec3(1, 0.2f, 1),
            PhysicsMaterial::iron()
        );
    }

    void on_update(float dt) override {
        t += forward ? dt * 0.5f : -dt * 0.5f;
        t = clamp(t, 0.0f, 1.0f);

        if (t >= 1.0f) { t = 1.0f; forward = false; }
        if (t <= 0.0f) { t = 0.0f; forward = true; }

        Vec3 pos = lerp(start_pos, end_pos, t);
        body->set_position(pos);
    }
};
```

Add the platform to your scene:

```cpp
void create_moving_platform() {
    auto container = create_child<MovingPlatform>();
    container->move_to(0, 0, 0);

    // Visual
    auto platform = container->create_child<Actor>();
    auto mesh = assets->new_mesh_from_procedural_cube();
    platform->set_mesh(mesh->id());
    platform->scale_by(2, 0.4f, 2);

    S_DEBUG("Moving platform created");
}
```

Drop a box on the platform and watch it get carried along!

---

## Step 9: Using Sphere Colliders and Capsules

Not everything is a box. Here is how to use other collider shapes:

### Sphere collider

```cpp
// Create a bouncing ball
auto ball = create_child<Actor>();
auto sphere_mesh = assets->new_mesh_from_procedural_sphere();
ball->set_mesh(sphere_mesh->id());
ball->move_to(0, 8, 0);

auto body = ball->create_child<DynamicBody>();
body->add_sphere_collider(1.0f, PhysicsMaterial::rubber());  // 1.0 = diameter
body->set_mass(0.5f);
body->set_restitution(0.9f);  // Very bouncy!
```

### Capsule collider (for characters)

```cpp
auto character = create_child<Actor>();
// ... visual setup ...

auto body = character->create_child<DynamicBody>();
body->add_capsule_collider(
    Vec3(0, -0.5f, 0),  // Bottom endpoint
    Vec3(0, 0.5f, 0),   // Top endpoint
    0.3f,                // Diameter
    PhysicsMaterial::wood()
);
```

---

## Step 10: Raycasting -- Querying the Physics World

Cast rays to find out what is in the physics world:

```cpp
void on_update(float dt) override {
    Scene::on_update(dt);

    auto input = window->input;
    if (input->is_button_down(BUTTON_A)) {
        // Cast a ray downward from above the scene
        Ray ray(Vec3(0, 20, 0), Vec3(0, -1, 0));
        PhysicsRaycastHit hit;

        auto sim = physics_simulation();

        if (sim->raycast(ray, 100.0f, hit)) {
            S_INFO("Hit something at distance: {}", hit.distance);
            S_INFO("Hit point: {}", hit.point);
            S_INFO("Hit normal: {}", hit.normal);
        } else {
            S_INFO("Ray did not hit anything");
        }
    }
}
```

### Multiple hits

```cpp
PhysicsRaycastOptions opts;
opts.hit_multiple = true;  // Get all hits, not just first

std::vector<PhysicsRaycastHit> hits;
sim->raycast_multi(ray, 100.0f, hits, opts);

for (const auto& hit : hits) {
    S_INFO("Hit at distance: {}", hit.distance);
}
```

---

## Complete Example

Here is the full working physics demo:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class BouncingBox : public StageNode {
public:
    FindResult<DynamicBody> body = FindDescendent<DynamicBody>(this);

    void on_load() override {
        body->signal_collision_enter().connect(
            [this](const Collision& collision) {
                S_DEBUG("Box collided with: {}", collision.other_collider_name);
            }
        );
    }
};

class PhysicsDemoScene : public Scene {
public:
    PhysicsDemoScene(Window* window) : Scene(window) {}

    void on_load() override {
        // Camera
        camera_ = create_child<Camera3D>({
            {"znear",  0.1f},
            {"zfar",   100.0f},
            {"aspect", window->aspect_ratio()},
            {"yfov",   45.0f}
        });

        camera_->set_perspective_projection(
            Degrees(45.0),
            window->aspect_ratio(),
            0.1f,
            100.0f
        );

        camera_->transform->set_position(Vec3(0, 8, 18));
        camera_->transform->look_at(Vec3(0, 2, 0));

        // Build the scene
        create_ground();
        create_falling_boxes();
        create_moving_platform();

        // Render layer
        auto layer = compositor->create_layer(this, camera_);
        layer->set_clear_flags(BUFFER_CLEAR_ALL);
        layer->viewport->set_color(Color(0.2f, 0.2f, 0.3f));

        S_DEBUG("Physics scene loaded");
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        // Press Space for an explosion
        auto input = window->input;
        if (input->is_button_down(BUTTON_SPACE)) {
            explosion(Vec3(0, 2, 0), 15.0f, 500.0f);
        }
    }

private:
    void create_ground() {
        auto ground = create_child<Actor>();
        auto mesh = assets->new_mesh_from_procedural_cube();
        ground->set_mesh(mesh->id());
        ground->scale_by(20, 0.5f, 20);
        ground->move_to(0, -1, 0);

        auto body = ground->create_child<StaticBody>();
        body->add_box_collider(Vec3(20, 0.5f, 20), PhysicsMaterial::stone());
    }

    void create_falling_boxes() {
        for (int i = 0; i < 5; ++i) {
            auto container = create_child<BouncingBox>();

            auto box = container->create_child<Actor>();
            auto mesh = assets->new_mesh_from_procedural_cube();
            box->set_mesh(mesh->id());

            float x = (i - 2) * 3.0f;
            container->move_to(x, 10 + i * 2, 0);

            auto body = box->create_child<DynamicBody>();
            body->add_box_collider(Vec3(0.5f, 0.5f, 0.5f), PhysicsMaterial::wood());
            body->set_mass(1.0f);
            body->set_restitution(0.5f);
        }
    }

    void create_moving_platform() {
        auto container = create_child<MovingPlatform>();

        auto platform = container->create_child<Actor>();
        auto mesh = assets->new_mesh_from_procedural_cube();
        platform->set_mesh(mesh->id());
        platform->scale_by(2, 0.4f, 2);
    }

    void explosion(Vec3 center, float radius, float force) {
        auto bodies = find_descendents_by_types({DynamicBody::Meta::node_type});
        for (auto* node : bodies) {
            auto* body = static_cast<DynamicBody*>(node);
            Vec3 pos = body->transform->position();
            float dist = (pos - center).length();
            if (dist < radius) {
                Vec3 dir = (pos - center).normalized();
                float strength = force * (1.0f - dist / radius);
                body->add_impulse(dir * strength);
            }
        }
    }

    Camera3D* camera_ = nullptr;
};

class MovingPlatform : public StageNode {
public:
    FindResult<KinematicBody> body = FindDescendent<KinematicBody>(this);
    Vec3 start_pos = Vec3(-5, 0, 0);
    Vec3 end_pos = Vec3(5, 0, 0);
    float t = 0.0f;
    bool forward = true;

    void on_load() override {
        body->add_box_collider(Vec3(1, 0.2f, 1), PhysicsMaterial::iron());
    }

    void on_update(float dt) override {
        t += forward ? dt * 0.5f : -dt * 0.5f;
        t = clamp(t, 0.0f, 1.0f);
        if (t >= 1.0f) { t = 1.0f; forward = false; }
        if (t <= 0.0f) { t = 0.0f; forward = true; }
        body->set_position(lerp(start_pos, end_pos, t));
    }
};

class PhysicsDemo : public Application {
public:
    PhysicsDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<PhysicsDemoScene>("main");
        scenes->activate("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Physics Demo";
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    PhysicsDemo app(config);
    return app.run(argc, argv);
}
```

---

## Choosing the Right Body Type

| Scenario | Body Type |
|----------|-----------|
| Falling crate | `DynamicBody` |
| Bouncing ball | `DynamicBody` |
| Character | `DynamicBody` or `KinematicBody` |
| Ground / floor | `StaticBody` |
| Walls | `StaticBody` |
| Moving platform | `KinematicBody` |
| Elevator | `KinematicBody` |
| Sliding door | `KinematicBody` |

---

## Best Practices

### 1. Always create the visual first, then attach physics

```cpp
auto actor = create_child<Actor>();
actor->set_mesh(mesh->id());

auto body = actor->create_child<DynamicBody>();
body->add_box_collider(size, material);
```

This ensures the physics body inherits the visual node's transform.

### 2. Use simple collider shapes

Primitive shapes (boxes, spheres, capsules) are much faster than mesh colliders. Use mesh colliders only for complex static geometry.

### 3. Physics bodies are destroyed automatically

When you destroy a node, its physics body is automatically removed from the simulation. You do not need to clean up manually.

### 4. Use appropriate materials

```cpp
PhysicsMaterial::wood()    // Crates, platforms (medium friction, slight bounce)
PhysicsMaterial::rubber()  // Balls (low friction, high bounce)
PhysicsMaterial::stone()   // Ground, walls (high friction, no bounce)
PhysicsMaterial::iron()    // Heavy objects (low friction, no bounce)
```

---

## Summary

| Concept | Key Methods |
|---------|------------|
| Create static body | `node->create_child<StaticBody>()` |
| Create dynamic body | `node->create_child<DynamicBody>()` |
| Create kinematic body | `node->create_child<KinematicBody>()` |
| Add box collider | `body->add_box_collider(half_extents, material)` |
| Add sphere collider | `body->add_sphere_collider(diameter, material)` |
| Add capsule collider | `body->add_capsule_collider(bottom, top, diameter, material)` |
| Apply force | `body->add_force(vec3)` |
| Apply impulse | `body->add_impulse(vec3)` |
| Set mass | `body->set_mass(float)` |
| Set bounciness | `body->set_restitution(float)` |
| Collision events | `body->signal_collision_enter().connect(...)` |
| Raycast | `sim->raycast(ray, distance, hit)` |

**Next:** [Tutorial 4 -- User Interface](04_user_interface.md)
