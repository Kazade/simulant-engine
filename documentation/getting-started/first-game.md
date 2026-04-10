# Your First Simulant Game

Welcome! This tutorial will guide you through creating your first Simulant game from scratch. We'll create a simple application that displays a 3D model and lets you look around.

> **Prerequisites**: You should have Simulant installed. If not, follow the [Installation Guide](installation.md).

## Step 1: Create a New Project

Open a terminal and run:

```bash
# Create a projects directory
mkdir -p ~/Projects
cd ~/Projects

# Create a new Simulant project called "my-first-game"
simulant start my-first-game
```

The `start` command will:
- Download the Simulant engine for your platform
- Set up the project structure
- Download core assets
- Configure the build system

This may take a minute. When it's done, you'll see a new directory:

```
my-first-game/
├── assets/          # Game assets (models, textures, sounds)
├── libraries/       # Simulant library
├── packages/        # Built packages for distribution
├── sources/         # Your C++ source code
├── tests/           # Unit tests
├── tools/           # Binary tools
├── CMakeLists.txt   # Build configuration
└── simulant.json    # Project configuration
```

## Step 2: Build and Run

Let's verify everything works:

```bash
cd my-first-game

# Build the project
simulant build

# Run the game
simulant run
```

You should see the Simulant splash screen, followed by a blank window. Press **Escape** to quit.

> **Tip**: Use `simulant run --rebuild` to rebuild and run in one command.

## Step 3: Understand the Project Structure

Open the project in your favorite editor. The main source file is:

```
sources/main.cpp
```

Let's look at the default code:

```cpp
#include <simulant/simulant.h>

using namespace smlt;

class MyGame : public Application {
public:
    MyGame(const AppConfig& config):
        Application(config) {}

    bool init() {
        // Scene registration happens here
        scenes->register_scene<InGameScene>("ingame");
        scenes->register_scene<InGameScene>("main"); // "main" is the starting scene
        return true;
    }
};

int main(int argc, char* argv[]) {
    AppConfig config;
    config.title = "My First Game";
    config.width = 1280;
    config.height = 720;

    MyGame app(config);
    return app.run(argc, argv);
}
```

Key points:
- Your game is an `Application` subclass
- The `init()` method is where you set up scenes
- Scenes are registered by name - `"main"` is the first scene loaded
- `AppConfig` controls window settings

## Step 4: Create Your First Scene

Scenes represent different parts of your game (menu, gameplay, game over, etc.). Let's create a simple gameplay scene.

Create a new file `sources/ingame_scene.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

using namespace smlt;

class InGameScene : public Scene<InGameScene> {
public:
    InGameScene(Window* window):
        Scene<InGameScene>(window) {}

    void on_load() override;
    void on_update(float dt) override;
};
```

Now create `sources/ingame_scene.cpp`:

```cpp
#include "ingame_scene.h"

void InGameScene::on_load() {
    // This is where we build our scene
    S_INFO("InGameScene loaded!");
}

void InGameScene::on_update(float dt) {
    // Called every frame
    // dt is the time since the last frame in seconds
}
```

Now update your `main.cpp` to include the new scene:

```cpp
#include <simulant/simulant.h>
#include "ingame_scene.h"  // Add this

using namespace smlt;

class MyGame : public Application {
public:
    MyGame(const AppConfig& config):
        Application(config) {}

    bool init() {
        scenes->register_scene<InGameScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    AppConfig config;
    config.title = "My First Game";
    config.width = 1280;
    config.height = 720;

    MyGame app(config);
    return app.run(argc, argv);
}
```

Build and run:

```bash
simulant run --rebuild
```

You should see a blank window with "InGameScene loaded!" in the console output.

## Step 5: Add a Camera

Every visible scene needs a camera. Let's add one to our scene.

Update `ingame_scene.h`:

```cpp
#pragma once

#include <simulant/simulant.h>

using namespace smlt;

class InGameScene : public Scene<InGameScene> {
public:
    InGameScene(Window* window):
        Scene<InGameScene>(window) {}

    void on_load() override;
    void on_update(float dt) override;

private:
    CameraID camera_;
};
```

Update `ingame_scene.cpp`:

```cpp
#include "ingame_scene.h"

void InGameScene::on_load() {
    S_INFO("InGameScene loaded!");

    // Create a 3D camera
    camera_ = create_child<Camera3D>()->id();
    
    // Position the camera
    auto camera_ptr = camera(camera_);
    camera_ptr->move_to(0, 5, -10);
    camera_ptr->rotate_to(Degrees(20), Degrees(0), Degrees(0));
}

void InGameScene::on_update(float dt) {
    // Called every frame
}
```

## Step 6: Add a Render Pipeline

A camera alone won't show anything - we need a render pipeline (called a **Layer** in the compositor).

Update `ingame_scene.cpp`:

```cpp
#include "ingame_scene.h"

void InGameScene::on_load() {
    S_INFO("InGameScene loaded!");

    // Create a 3D camera
    camera_ = create_child<Camera3D>()->id();
    
    // Position the camera
    auto camera_ptr = camera(camera_);
    camera_ptr->move_to(0, 5, -10);
    camera_ptr->rotate_to(Degrees(20), Degrees(0), Degrees(0));

    // Create a render pipeline
    // This tells the compositor: "Render this stage from this camera"
    auto layer = compositor->create_layer(this, camera(camera_));
    layer->activate();
    
    // Link the pipeline to this scene so it auto-activates/deactivates
    link_pipeline(layer);
}
```

Build and run. You should see a blank scene with a dark background. We have a camera, but nothing to look at!

## Step 7: Add a 3D Object

Let's add a simple cube to the scene. Simulant includes procedural mesh generation, so we don't need external assets yet.

Add a member variable to `ingame_scene.h`:

```cpp
private:
    CameraID camera_;
    ActorID cube_actor_;
```

Update `ingame_scene.cpp`:

```cpp
#include "ingame_scene.h"

void InGameScene::on_load() {
    S_INFO("InGameScene loaded!");

    // Create a 3D camera
    camera_ = create_child<Camera3D>()->id();
    
    auto camera_ptr = camera(camera_);
    camera_ptr->move_to(0, 5, -10);
    camera_ptr->rotate_to(Degrees(20), Degrees(0), Degrees(0));

    // Create a cube mesh procedurally
    auto cube_mesh = assets->new_mesh_from_procedural_cube();
    
    // Create an actor to render the mesh
    cube_actor_ = create_child<Actor>()->id();
    auto actor = actor(cube_actor_);
    actor->set_mesh(cube_mesh->id());
    actor->move_to(0, 0, 0);

    // Create a render pipeline
    auto layer = compositor->create_layer(this, camera(camera_));
    layer->activate();
    link_pipeline(layer);
}
```

Build and run. You should now see a white cube in the center of the screen!

## Step 8: Make It Move

Static cubes are boring. Let's make it rotate!

Add a member variable:

```cpp
private:
    CameraID camera_;
    ActorID cube_actor_;
    float rotation_angle_ = 0.0f;
```

Update the `on_update` method:

```cpp
void InGameScene::on_update(float dt) {
    // Rotate the cube over time
    rotation_angle_ += dt * 90.0f; // 90 degrees per second
    
    if (auto actor = maybe_actor(cube_actor_)) {
        actor->rotate_to(Degrees(0), Degrees(rotation_angle_), Degrees(0));
    }
}
```

The `maybe_actor()` method safely returns an optional pointer - if the actor was destroyed, it returns empty rather than crashing.

Build and run. Your cube should now be spinning!

## Step 9: Handle Input

Let's make the camera move with the arrow keys.

Update `ingame_scene.cpp`:

```cpp
#include "ingame_scene.h"

void InGameScene::on_load() {
    S_INFO("InGameScene loaded!");

    // Create a 3D camera
    camera_ = create_child<Camera3D>()->id();
    
    auto camera_ptr = camera(camera_);
    camera_ptr->move_to(0, 5, -10);
    camera_ptr->rotate_to(Degrees(20), Degrees(0), Degrees(0));

    // Define input axes
    auto move_left_axis = input->new_axis("move_left");
    move_left_axis->set_positive_keyboard_key(KEYBOARD_CODE_LEFT);
    
    auto move_right_axis = input->new_axis("move_right");
    move_right_axis->set_positive_keyboard_key(KEYBOARD_CODE_RIGHT);
    
    auto move_forward_axis = input->new_axis("move_forward");
    move_forward_axis->set_positive_keyboard_key(KEYBOARD_CODE_UP);
    
    auto move_back_axis = input->new_axis("move_back");
    move_back_axis->set_positive_keyboard_key(KEYBOARD_CODE_DOWN);

    // Create a cube mesh procedurally
    auto cube_mesh = assets->new_mesh_from_procedural_cube();
    
    // Create an actor to render the mesh
    cube_actor_ = create_child<Actor>()->id();
    auto actor = actor(cube_actor_);
    actor->set_mesh(cube_mesh->id());
    actor->move_to(0, 0, 0);

    // Create a render pipeline
    auto layer = compositor->create_layer(this, camera(camera_));
    layer->activate();
    link_pipeline(layer);
}

void InGameScene::on_update(float dt) {
    // Rotate the cube
    rotation_angle_ += dt * 90.0f;
    
    if (auto actor = maybe_actor(cube_actor_)) {
        actor->rotate_to(Degrees(0), Degrees(rotation_angle_), Degrees(0));
    }

    // Move the camera
    if (auto cam = maybe_camera(camera_)) {
        float speed = 5.0f * dt;
        
        if (input->axis_value("move_left") > 0) {
            cam->move_by(-speed, 0, 0);
        }
        if (input->axis_value("move_right") > 0) {
            cam->move_by(speed, 0, 0);
        }
        if (input->axis_value("move_forward") > 0) {
            cam->move_by(0, 0, speed);
        }
        if (input->axis_value("move_back") > 0) {
            cam->move_by(0, 0, -speed);
        }
    }
}
```

Build and run. Use the arrow keys to move the camera around the spinning cube!

## Step 10: Add a Material

White cubes are dull. Let's add color with a material.

Add to `on_load()` before creating the actor:

```cpp
    // Create a red material
    auto material = assets->new_material();
    material->set_diffuse(Colour(1.0f, 0.0f, 0.0f, 1.0f)); // Red

    // Create an actor to render the mesh
    cube_actor_ = create_child<Actor>()->id();
    auto actor = actor(cube_actor_);
    actor->set_mesh(cube_mesh->id());
    actor->set_material(material->id());
    actor->move_to(0, 0, 0);
```

Build and run. Your cube should now be red!

## What We've Learned

Congratulations! You've created your first Simulant game. Here's what you learned:

1. **Applications** - Entry point to your game
2. **Scenes** - Manage game state with `on_load()` and `on_update()`
3. **Cameras** - Viewpoints into the 3D world
4. **Layers** - Render pipelines for the compositor
5. **Actors** - Objects that render meshes
6. **Materials** - Control appearance with colors
7. **Input** - Read keyboard input with axes
8. **Safe Access** - Use `maybe_actor()` and `maybe_camera()` for safety

## What's Next?

- **[Project Structure](project-structure.md)** - Understand Simulant projects deeply
- **[Scenes](../core-concepts/scenes.md)** - Learn more about scene management
- **[Actors & Meshes](../core-concepts/actors.md)** - Load 3D models from files
- **[Physics](../physics/overview.md)** - Add physics simulation
- **[UI](../ui/overview.md)** - Build user interfaces

## Complete Code

Here's the final code for reference:

### `sources/ingame_scene.h`

```cpp
#pragma once

#include <simulant/simulant.h>

using namespace smlt;

class InGameScene : public Scene<InGameScene> {
public:
    InGameScene(Window* window):
        Scene<InGameScene>(window) {}

    void on_load() override;
    void on_update(float dt) override;

private:
    CameraID camera_;
    ActorID cube_actor_;
    float rotation_angle_ = 0.0f;
};
```

### `sources/ingame_scene.cpp`

```cpp
#include "ingame_scene.h"

void InGameScene::on_load() {
    S_INFO("InGameScene loaded!");

    // Create a 3D camera
    camera_ = create_child<Camera3D>()->id();
    
    auto camera_ptr = camera(camera_);
    camera_ptr->move_to(0, 5, -10);
    camera_ptr->rotate_to(Degrees(20), Degrees(0), Degrees(0));

    // Define input axes
    auto move_left_axis = input->new_axis("move_left");
    move_left_axis->set_positive_keyboard_key(KEYBOARD_CODE_LEFT);
    
    auto move_right_axis = input->new_axis("move_right");
    move_right_axis->set_positive_keyboard_key(KEYBOARD_CODE_RIGHT);
    
    auto move_forward_axis = input->new_axis("move_forward");
    move_forward_axis->set_positive_keyboard_key(KEYBOARD_CODE_UP);
    
    auto move_back_axis = input->new_axis("move_back");
    move_back_axis->set_positive_keyboard_key(KEYBOARD_CODE_DOWN);

    // Create a cube mesh procedurally
    auto cube_mesh = assets->new_mesh_from_procedural_cube();
    
    // Create a red material
    auto material = assets->new_material();
    material->set_diffuse(Colour(1.0f, 0.0f, 0.0f, 1.0f));

    // Create an actor to render the mesh
    cube_actor_ = create_child<Actor>()->id();
    auto actor = actor(cube_actor_);
    actor->set_mesh(cube_mesh->id());
    actor->set_material(material->id());
    actor->move_to(0, 0, 0);

    // Create a render pipeline
    auto layer = compositor->create_layer(this, camera(camera_));
    layer->activate();
    link_pipeline(layer);
}

void InGameScene::on_update(float dt) {
    // Rotate the cube
    rotation_angle_ += dt * 90.0f;
    
    if (auto actor = maybe_actor(cube_actor_)) {
        actor->rotate_to(Degrees(0), Degrees(rotation_angle_), Degrees(0));
    }

    // Move the camera
    if (auto cam = maybe_camera(camera_)) {
        float speed = 5.0f * dt;
        
        if (input->axis_value("move_left") > 0) {
            cam->move_by(-speed, 0, 0);
        }
        if (input->axis_value("move_right") > 0) {
            cam->move_by(speed, 0, 0);
        }
        if (input->axis_value("move_forward") > 0) {
            cam->move_by(0, 0, speed);
        }
        if (input->axis_value("move_back") > 0) {
            cam->move_by(0, 0, -speed);
        }
    }
}
```
