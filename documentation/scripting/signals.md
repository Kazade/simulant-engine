# Signals and Events

Signals provide a type-safe, flexible mechanism for implementing event-driven programming in Simulant. They allow objects to communicate without being tightly coupled to each other -- a sender (the signal) does not need to know which objects are listening, and listeners (slots) can be attached or detached at any time.

**Related documentation:**

- [Application](../core-concepts/application.md) -- for application lifecycle signals
- [Stage Nodes](../core-concepts/stage-nodes.md) -- for node destruction signals
- [Coroutines](coroutines.md) -- for combining signals with async tasks

---

## Table of Contents

1. [What Are Signals and Why Use Them?](#1-what-are-signals-and-why-use-them)
2. [Signal/Slot Architecture](#2-signalslot-architecture)
3. [Creating Signals](#3-creating-signals)
4. [Connecting to Signals](#4-connecting-to-signals)
5. [Disconnecting from Signals](#5-disconnecting-from-signals)
6. [Emitting Signals](#6-emitting-signals)
7. [Built-in Signals in Simulant](#7-built-in-signals-in-simulant)
8. [One-Shot Connections](#8-one-shot-connections)
9. [Signal Performance Considerations](#9-signal-performance-considerations)
10. [Common Patterns and Best Practices](#10-common-patterns-and-best-practices)
11. [Anti-Patterns and Pitfalls](#11-anti-patterns-and-pitfalls)
12. [Complete Examples](#12-complete-examples)

---

## 1. What Are Signals and Why Use Them?

In traditional procedural code, object A that wants to notify object B must hold a direct pointer to B and call a method on it. This creates **tight coupling** -- A must know the concrete type of B, and changing the relationship later requires modifying A's code.

Event-driven programming reverses this: A announces that something happened ("an event occurred"), and any interested party can listen for that announcement. This is the role of the signal/slot pattern.

### Benefits

- **Decoupling**: The emitter does not know who is listening.
- **Flexibility**: Listeners can be added or removed at runtime.
- **One-to-many**: A single signal can notify arbitrarily many listeners.
- **Type safety**: The compiler enforces that the callback signature matches the signal's signature.
- **Testability**: Signals make it easy to verify that events occur in unit tests.

### When to Use Signals

- Responding to user interface events (button clicks, keyboard input).
- Reacting to physics events (collision enter/exit).
- Observing lifecycle events (node creation, destruction).
- Hooking into the game loop (frame started, update, shutdown).
- Any situation where multiple unrelated systems need to react to the same event.

---

## 2. Signal/Slot Architecture

Simulant's signal system lives in the `smlt::sig` namespace and is defined in `<simulant/signals/signal.h>`. The architecture consists of four key components:

| Component | Description |
|---|---|
| **`signal<Signature>`** | The signal object. Holds a list of connected callbacks. |
| **`Connection`** | A handle representing a single connection. Can be disconnected later. |
| **`ScopedConnection`** | A RAII-style connection that automatically disconnects when it goes out of scope. |
| **`DEFINE_SIGNAL` macro** | A convenience macro for declaring signals as class members. |

### How It Works

1. A class declares a `signal<T>` member (or uses `DEFINE_SIGNAL`).
2. Other code connects a callback (lambda, function pointer, or bound member function) to the signal.
3. When the event occurs, the owning class **emits** the signal by calling `operator()`.
4. All connected callbacks are invoked in the order they were connected.

```cpp
// Simplified flow:
//
//   [Emitter Object]
//        |
//        |-- signal<void(int)> my_signal
//        |       |
//        |       +--- callback 1: [](int x) { ... }
//        |       +--- callback 2: some_function
//        |       +--- callback 3: [this](int x) { ... }
//        |
//   my_signal(42);   <-- emits, all three callbacks fire
```

---

## 3. Creating Signals

### Basic Signal Declaration

A signal is templated on a function signature. The signature defines what arguments the signal will pass to its listeners.

```cpp
#include <simulant/signals/signal.h>

using namespace smlt;

// A signal with no arguments
sig::signal<void()> on_started;

// A signal that passes a float (e.g., delta time)
sig::signal<void(float)> on_update;

// A signal that passes two objects
sig::signal<void(StageNode*, StageNodeType)> on_node_created;

// A signal that returns a value (rare; see note below)
sig::signal<bool(const std::string&)> on_text_input;
```

### Using `DEFINE_SIGNAL` in Classes

Simulant provides the `DEFINE_SIGNAL` macro to declare a signal as a class member with a getter method. This is the idiomatic approach throughout the engine:

```cpp
class MyComponent {
    // Macro expands to:
    //   public:  UpdatedSignal& signal_update() { return signal_update_; }
    //            UpdatedSignal& signal_update() const { return signal_update_; }
    //   private: mutable UpdatedSignal signal_update_;
    DEFINE_SIGNAL(UpdatedSignal, signal_update);
};
```

You then define the signal type alias at namespace scope:

```cpp
typedef sig::signal<void(float)> UpdatedSignal;
```

The macro gives you a clean public accessor (`signal_update()`) while keeping the actual member (`signal_update_`) private. The member is declared `mutable` so it can be accessed through `const` methods.

### Naming Convention

Signal accessor methods follow the pattern `signal_<past_tense_verb>()` or `signal_<event_name>()`:

- `signal_destroyed()`
- `signal_collision_enter()`
- `signal_activated()`
- `signal_update()`

---

## 4. Connecting to Signals

Connecting a callback to a signal returns a `Connection` handle. There are several ways to provide callbacks.

### 4.1. Lambda Functions

Lambdas are the most common approach. They are concise and can capture local state.

```cpp
app->signal_update().connect([](float dt) {
    // dt is the delta time since the last frame
    std::cout << "Frame delta time: " << dt << std::endl;
});
```

With captures:

```cpp
int frame_count = 0;
app->signal_frame_started().connect([&frame_count]() {
    ++frame_count;
    if (frame_count % 60 == 0) {
        std::cout << "One minute elapsed (at 60fps)" << std::endl;
    }
});
```

### 4.2. Member Functions

To connect a member function, use `std::bind` or a capturing lambda:

```cpp
class Player {
public:
    Player(Application* app) {
        // Using a capturing lambda (recommended, clearest approach)
        app->signal_shutdown().connect([this]() {
            this->on_application_shutdown();
        });
    }

private:
    void on_application_shutdown() {
        save_game();
    }
};
```

### 4.3. Free Functions

```cpp
void handle_shutdown() {
    std::cout << "Application is shutting down!" << std::endl;
}

app->signal_shutdown().connect(&handle_shutdown);
```

### 4.4. Connections with Return Values

Some signals expect a return value from listeners. The text input signal, for example, returns `bool`:

```cpp
// signal_text_input_received has signature:
//   sig::signal<bool(const unicode&, TextInputEvent&)>

input->signal_text_input_received().connect(
    [](const unicode& c, TextInputEvent& evt) -> bool {
        // Return true to accept the character, false to reject it
        if (c == '@') {
            return false;  // Block this character
        }
        text_buffer += c;
        return true;
    }
);
```

**Important**: When a signal has a return type, the signal's `operator()` returns the value from the **last** connected callback. There is no built-in accumulation or combination of return values.

### 4.5. Storing Connection Handles

Always store the connection if you plan to disconnect it later:

```cpp
// Connection - manual lifetime management
sig::connection conn = button->signal_clicked().connect([&]() {
    on_button_clicked();
});

// Later:
conn.disconnect();
```

---

## 5. Disconnecting from Signals

### 5.1. Manual Disconnection

Call `disconnect()` on the `Connection` handle:

```cpp
sig::connection conn = app->signal_update().connect(callback);

// When you no longer need the connection:
conn.disconnect();

// Check if still connected:
if (conn.is_connected()) {
    std::cout << "Still connected" << std::endl;
}
```

### 5.2. Scoped Connections (RAII)

`ScopedConnection` automatically disconnects when it is destroyed. This is the safest approach when connections should live for the lifetime of a scope or object.

```cpp
{
    sig::scoped_connection conn = button->signal_clicked().connect([&]() {
        handle_click();
    });

    // conn is active here
    assert(conn.is_connected());

} // <-- conn goes out of scope, disconnect() called automatically

// conn is now disconnected
```

**Scoped connections are strongly recommended** when the lifetime of the connection is tied to a scope or object lifetime. They prevent dangling callbacks that could access destroyed objects.

### 5.3. Disconnecting Inside a Callback

It is safe to disconnect from within a callback. The signal implementation uses deferred removal -- dead connections are marked during iteration and removed after the signal finishes emitting.

```cpp
sig::connection conn;
conn = app->signal_update().connect([&](float dt) {
    total_dt += dt;
    if (total_dt >= 5.0f) {
        // Safe: disconnects itself after this call completes
        conn.disconnect();
    }
});
```

### 5.4. Checking Connection Validity

```cpp
if (conn) {
    // Equivalent to conn.is_connected()
}
```

### 5.5. Connection Safety

If a signal object is destroyed while connections still point to it, those connections become inert -- calling `disconnect()` or `is_connected()` on them will safely return `false`. This prevents crashes from stale connections.

---

## 6. Emitting Signals

Emitting a signal is done by calling the signal's `operator()`:

```cpp
class MyClass {
    DEFINE_SIGNAL(DestroyedSignal, signal_destroyed);

public:
    bool destroy() {
        if (already_destroyed_) return false;
        already_destroyed_ = true;

        // Emit the signal -- all connected callbacks fire
        signal_destroyed()();

        // ... perform destruction ...
        return true;
    }
};
```

For signals with arguments, pass the arguments when calling:

```cpp
// signal_update has signature sig::signal<void(float)>
signal_update_(dt);

// signal_stage_node_created has signature
//   sig::signal<void(StageNode*, StageNodeType)>
signal_stage_node_created_(new_node, node_type);

// signal_collision_enter has signature
//   sig::signal<void(const Collision&)>
signal_collision_enter_(collision_data);
```

### Emission Order

Callbacks are invoked in the order they were connected. If you need a specific order, connect in that order.

---

## 7. Built-in Signals in Simulant

Simulant provides many signals across its core classes. Below is a comprehensive reference.

### 7.1. Application Signals

**Source:** `Application` class (see [Application docs](../core-concepts/application.md))

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_frame_started()` | `void()` | At the very beginning of each frame. |
| `signal_update(float)` | `void(float)` | During the update phase, passing delta time. |
| `signal_fixed_update(float)` | `void(float)` | During fixed-update phase, passing fixed timestep. |
| `signal_late_update(float)` | `void(float)` | After all updates, before coroutines resume. |
| `signal_post_late_update()` | `void()` | After late update completes. |
| `signal_post_coroutines()` | `void()` | After coroutines have been resumed. |
| `signal_frame_finished()` | `void()` | At the end of each frame, before swap. |
| `signal_pre_swap()` | `void()` | Just before the back buffer is swapped. |
| `signal_shutdown()` | `void()` | When the application is shutting down. |

```cpp
// Example: Frame timing with application signals
class FrameTimer {
public:
    FrameTimer(Application* app) {
        app->signal_frame_started().connect([this]() {
            frame_start = std::chrono::high_resolution_clock::now();
        });
        app->signal_frame_finished().connect([this]() {
            auto end = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - frame_start).count();
            if (ms > 16) {
                S_WARN("Frame took {}ms, target is 16ms (60fps)", ms);
            }
        });
    }

private:
    std::chrono::high_resolution_clock::time_point frame_start;
};
```

### 7.2. Scene Signals

**Source:** `Scene` class

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_activated()` | `void()` | When the scene becomes active. |
| `signal_deactivated()` | `void()` | When the scene is deactivated. |
| `signal_stage_node_created(StageNode*, StageNodeType)` | `void(StageNode*, StageNodeType)` | When a new node is added to the scene. |
| `signal_stage_node_destroyed(StageNode*, StageNodeType)` | `void(StageNode*, StageNodeType)` | When a node is destroyed. |
| `signal_layer_render_started(Camera*, Viewport*, StageNode*)` | `void(Camera*, Viewport*, StageNode*)` | Before a layer renders. |
| `signal_layer_render_finished(Camera*, Viewport*, StageNode*)` | `void(Camera*, Viewport*, StageNode*)` | After a layer renders. |

```cpp
// Example: Log all node destructions for debugging
scene->signal_stage_node_destroyed().connect(
    [](StageNode* node, StageNodeType type) {
        S_DEBUG("Node destroyed: type={}, name={}",
                static_cast<int>(type), node->name());
    }
);
```

### 7.3. Scene Manager Signals

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_scene_activated(std::string, Scene*)` | `void(std::string, Scene*)` | When a scene is activated (passing route and scene). |
| `signal_scene_deactivated(std::string, Scene*)` | `void(std::string, Scene*)` | When a scene is deactivated. |

### 7.4. StageNode Signals

**Source:** `StageNode` class (see [Stage Nodes docs](../core-concepts/stage-nodes.md))

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_destroyed()` | `void()` | Immediately when `destroy()` is called. |
| `signal_cleaned_up()` | `void()` | Just before the node is actually deleted (after `late_update`). |
| `signal_bounds_updated(AABB)` | `void(AABB)` | When the node's bounding box changes. |

```cpp
// Understanding destroy vs. clean_up:
auto actor = scene->create_child<smlt::Actor>(mesh);

bool destroyed = false;
bool cleaned_up = false;

actor->signal_destroyed().connect([&]() {
    destroyed = true;  // Fires immediately on destroy()
});

actor->signal_cleaned_up().connect([&]() {
    cleaned_up = true;  // Fires later, just before deletion
});

actor->destroy();
// destroyed == true, cleaned_up == false

app->run_frame();
// Now cleaned_up == true (clean-up happens after late_update)
```

### 7.5. Widget Signals

**Source:** `Widget` class (see [UI docs](../ui/overview.md))

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_pressed()` | `void()` | When the widget is pressed/finger-down. |
| `signal_released()` | `void()` | When the finger/mouse is released over the widget. |
| `signal_clicked()` | `void()` | On finger-up (press then release on same widget). |
| `signal_focused()` | `void()` | When the widget gains input focus. |
| `signal_blurred()` | `void()` | When the widget loses input focus. |

### 7.6. Keyboard Signals

**Source:** `SoftKeyboard` class

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_key_pressed(SoftKeyPressedEvent&)` | `void(SoftKeyPressedEvent&)` | When a key is pressed on the on-screen keyboard. |
| `signal_done(const unicode&)` | `void(const unicode&)` | When the user confirms text input. |
| `signal_cancelled()` | `void()` | When the user cancels text input. |

### 7.7. Physics Signals

**Source:** `PhysicsBody` class (see [Physics docs](../physics/overview.md))

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_collision_enter(const Collision&)` | `void(const Collision&)` | When this body starts colliding with another. |
| `signal_collision_exit(const Collision&)` | `void(const Collision&)` | When this body stops colliding with another. |

```cpp
// Example: Track collisions on a physics body
body->signal_collision_enter().connect([&](const Collision& c) {
    S_INFO("Collision started with body: {}", c.other->id());
    flash_material(c.self);
});

body->signal_collision_exit().connect([&](const Collision& c) {
    S_INFO("Collision ended with body: {}", c.other->id());
    restore_material(c.self);
});
```

### 7.8. Animation Signals

**Source:** `KeyFrameAnimated` class (see [Animation docs](../animation/overview.md))

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_animation_added(KeyFrameAnimated*, const std::string&)` | `void(KeyFrameAnimated*, const std::string&)` | When an animation sequence is added. |

### 7.9. Audio Signals

**Source:** `AudioSource` class

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_sound_played(SoundPtr, AudioRepeat, DistanceModel)` | `void(SoundPtr, AudioRepeat, DistanceModel)` | When a sound starts playing. |
| `signal_stream_finished()` | `void()` | When an audio stream finishes. |

### 7.10. Window Signals

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_screen_added(std::string, Screen*)` | `void(std::string, Screen*)` | When a screen is added to the window. |
| `signal_screen_removed(std::string, Screen*)` | `void(std::string, Screen*)` | When a screen is removed. |
| `signal_focus_lost()` | `void()` | When the window loses OS focus. |
| `signal_focus_gained()` | `void()` | When the window gains OS focus. |

### 7.11. Input Manager Signals

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_text_input_received(const unicode&, TextInputEvent&)` | `bool(const unicode&, TextInputEvent&)` | When text input is received. Return `true` to accept. |

### 7.12. Updateable Signals

**Source:** `Updateable` mixin (used by `StageNode` and others)

| Signal | Signature | When It Fires |
|---|---|---|
| `signal_update(float)` | `void(float)` | Every frame during the update phase. |
| `signal_late_update(float)` | `void(float)` | Every frame during the late update phase. |
| `signal_fixed_update(float)` | `void(float)` | Every fixed timestep during the fixed update phase. |

### 7.13. Other Notable Signals

| Source | Signal | Signature | When It Fires |
|---|---|---|---|
| `DestroyableObject` | `signal_destroyed()` | `void()` | When `destroy()` is called. |
| `Compositor` | `signal_layer_render_started(Layer&)` | `void(Layer&)` | Before a compositor layer renders. |
| `Compositor` | `signal_layer_render_finished(Layer&)` | `void(Layer&)` | After a compositor layer renders. |
| `Mesh` | `signal_skeleton_added(Skeleton*)` | `void(Skeleton*)` | When a skeleton is attached to a mesh. |
| `Mesh` | `signal_submesh_created(AssetID, SubMeshPtr)` | `void(AssetID, SubMeshPtr)` | When a submesh is created. |
| `Mesh` | `signal_submesh_destroyed(AssetID, SubMeshPtr)` | `void(AssetID, SubMeshPtr)` | When a submesh is destroyed. |
| `Mesh` | `signal_submesh_material_changed(...)` | `void(AssetID, SubMeshPtr, MaterialSlot, AssetID, AssetID)` | When a submesh material changes. |
| `SubMesh` | `signal_material_changed(...)` | `void(SubMeshPtr, MaterialSlot, AssetID, AssetID)` | When the submesh's material changes. |
| `Actor` | `signal_mesh_changed(StageNodeID)` | `void(StageNodeID)` | When the actor's mesh changes. |
| `ParticleSystem` | `signal_material_changed(...)` | `void(ParticleSystem*, AssetID, AssetID)` | When the particle system material changes. |
| `AudioSource` | `signal_sound_played(...)` | `void(SoundPtr, AudioRepeat, DistanceModel)` | When a sound starts playing. |
| `AudioSource` | `signal_stream_finished()` | `void()` | When an audio stream finishes. |
| `Renderable` | `signal_render_priority_changed(old, new)` | `void(RenderPriority, RenderPriority)` | When render priority changes. |
| `VertexData` | `signal_update_complete()` | `void()` | When vertex data update is complete. |
| `GPUProgram` | `signal_linked()` | `void()` | When the GPU program links successfully. |
| `GPUProgram` | `signal_shader_compiled(ShaderType)` | `void(ShaderType)` | When a shader compiles. |
| `KeyFrameAnimated` | `signal_animation_added(...)` | `void(KeyFrameAnimated*, const std::string&)` | When an animation is added. |
| `Mesh` | `signal_animation_enabled(...)` | `void(Mesh*, MeshAnimationType, uint32_t)` | When an animation is enabled on a mesh. |
| `GenericTree` | `signal_parent_changed(...)` | `void(GenericTreeNode*, GenericTreeNode*)` | When a node's parent changes. |

---

## 8. One-Shot Connections

The signal system provides `connect_once()` for callbacks that should fire exactly once and then automatically disconnect:

```cpp
// This callback fires on the next update, then disconnects itself
app->signal_update().connect_once([&](float dt) {
    S_INFO("This only runs once!");
});
```

**Note:** The current implementation of `connect_once()` captures the callback and disconnects after the first invocation. For signals with parameters, be aware that the internal implementation wraps your callback and may have limitations with non-void return types. For more complex one-shot patterns, manually storing and disconnecting a `Connection` is often clearer:

```cpp
// Alternative: manual one-shot (works with any signature)
sig::connection conn;
conn = app->signal_update().connect([&](float dt) {
    S_INFO("Runs once with dt = {}", dt);
    conn.disconnect();
});
```

---

## 9. Signal Performance Considerations

### Implementation Characteristics

Simulant's signal implementation uses a **linked list** of callbacks. Understanding this helps you reason about performance:

- **Connect**: O(1) -- appended to the tail of the list.
- **Emit**: O(n) -- iterates all connected callbacks sequentially.
- **Disconnect**: O(n) -- searches the list for the matching connection.
- **Thread safety**: The implementation is **not thread-safe**. Signals should only be emitted and modified from a single thread (typically the main thread).

### Guidelines

1. **Minimize connections in hot paths.** Every additional listener adds a function call during emission. If a signal fires every frame and has 50 listeners, that is 50 function calls per frame.

2. **Keep callbacks short.** Since callbacks fire synchronously during emission, a slow callback blocks all subsequent callbacks and the rest of the game loop.

3. **Prefer `ScopedConnection` when possible.** The overhead is negligible, and it prevents memory-access bugs from stale callbacks.

4. **Avoid creating/destroying connections every frame.** Connection management has overhead. Set up connections once during initialization rather than in per-frame code.

5. **Be cautious with signals in tight loops.** If you are processing thousands of entities per frame, a per-entity signal emission can become a bottleneck. Consider batching or event queues instead.

---

## 10. Common Patterns and Best Practices

### 10.1. Self-Disconnecting Watcher Pattern

Watch for the destruction of an object you hold a reference to, and clear the reference automatically:

```cpp
class FindResult {
public:
    // ...
    T* get() const {
        if (!checked_) {
            found_ = dynamic_cast<T*>(finder_(node_));
            if (found_ && !found_->is_destroyed()) {
                // Auto-clear when the target is destroyed
                on_destroy_ = found_->signal_destroyed().connect([&]() {
                    found_ = nullptr;
                    on_destroy_.disconnect();
                });
            }
            checked_ = true;
        }
        return found_;
    }

private:
    mutable T* found_ = nullptr;
    mutable bool checked_ = false;
    sig::connection on_destroy_;
};
```

This pattern is used extensively in Simulant's `NodeLocator` system.

### 10.2. Debug Frame Counter

Use frame signals for quick debugging without modifying game logic:

```cpp
class DebugOverlay {
public:
    DebugOverlay(Application* app) {
        int frame_count = 0;
        float fps_timer = 0.0f;

        app->signal_frame_started().connect([&]() {
            ++frame_count;
            fps_timer += app->last_frame_time();

            if (fps_timer >= 1.0f) {
                S_INFO("FPS: {}", frame_count);
                frame_count = 0;
                fps_timer = 0.0f;
            }
        });
    }
};
```

### 10.3. Lifecycle Manager

Centralize shutdown logic using signals:

```cpp
class GameLifecycle {
public:
    GameLifecycle(Application* app) {
        app->signal_shutdown().connect([this]() {
            save_player_progress();
            close_network_connection();
            flush_audio_buffer();
        });
    }

private:
    void save_player_progress() { /* ... */ }
    void close_network_connection() { /* ... */ }
    void flush_audio_buffer() { /* ... */ }
};
```

### 10.4. Updateable Mixin Pattern

Any class that inherits from `Updateable` automatically gets per-frame signals:

```cpp
class Enemy : public StageNode {
public:
    Enemy(Scene* scene) : StageNode(scene, STAGE_NODE_TYPE_ENEMY) {
        // StageNode inherits Updateable, so we get signal_update
        signal_update().connect([this](float dt) {
            update_ai(dt);
        });
    }

private:
    void update_ai(float dt) {
        // AI logic runs every frame
    }
};
```

### 10.5. Testing with Signals

Signals make unit testing straightforward -- connect a lambda and verify it was called:

```cpp
void test_node_destruction_emits_signal() {
    auto actor = scene->create_child<smlt::Actor>(mesh);

    bool destroyed = false;
    sig::scoped_connection conn = actor->signal_destroyed().connect([&]() {
        destroyed = true;
    });

    actor->destroy();
    assert_true(destroyed);
}
```

### 10.6. Chaining Signals

You can forward a signal from one source through another:

```cpp
class GameScene : public Scene {
public:
    // Expose a signal that forwards the application's update signal
    sig::signal<void(float)>& signal_game_update() {
        return signal_game_update_;
    }

    bool init() override {
        // Forward app update to game-specific signal
        app->signal_update().connect([this](float dt) {
            signal_game_update_(dt);
        });
        return true;
    }

private:
    sig::signal<void(float)> signal_game_update_;
};
```

---

## 11. Anti-Patterns and Pitfalls

### 11.1. Dangling References in Captures

**Problem:** Capturing references to local variables that go out of scope.

```cpp
// BAD: local_variable is destroyed when the function returns
void setup_callback() {
    int local_variable = 42;
    app->signal_update().connect([&local_variable](float) {
        // CRASH: local_variable was destroyed!
        std::cout << local_variable << std::endl;
    });
}
```

**Fix:** Capture by value, or ensure the captured data outlives the connection.

```cpp
// GOOD: captured by value
void setup_callback() {
    int local_variable = 42;
    app->signal_update().connect([local_variable](float) {
        std::cout << local_variable << std::endl;
    });
}
```

### 11.2. Infinite Recursion

**Problem:** A signal callback emits the same signal, causing infinite recursion.

```cpp
// BAD: infinite loop!
signal_value_changed().connect([&](int value) {
    if (value > 100) {
        signal_value_changed(100);  // Calls itself recursively
    }
});
```

**Fix:** Use a guard flag or restructure the logic.

```cpp
bool emitting = false;
signal_value_changed().connect([&](int value) {
    if (emitting) return;  // Guard against recursion
    if (value > 100) {
        emitting = true;
        signal_value_changed_(100);
        emitting = false;
    }
});
```

### 11.3. Connecting Multiple Times

**Problem:** Calling `connect()` repeatedly without tracking, causing the callback to fire multiple times.

```cpp
// BAD: called every frame, adding a new listener each time
void on_frame() {
    button->signal_clicked().connect([&]() {
        handle_click();  // Will fire N times after N frames!
    });
}
```

**Fix:** Connect once during initialization.

```cpp
// GOOD: connect once in init
bool init() override {
    button->signal_clicked().connect([&]() {
        handle_click();
    });
    return true;
}
```

### 11.4. Assuming Callback Order

**Problem:** Relying on a specific order when multiple listeners are connected. While the current implementation calls them in connection order, this is an implementation detail that could change.

```cpp
// Fragile: assumes A fires before B
signal.connect([]() { /* A: setup */ });
signal.connect([]() { /* B: depends on A */ });
```

**Fix:** If order matters, combine into a single callback or document the dependency explicitly.

### 11.5. Not Disconnecting on Object Destruction

**Problem:** An object is destroyed but its signal connections remain, causing callbacks to access freed memory.

```cpp
class MyListener {
public:
    MyListener(Application* app) {
        app->signal_update().connect([this](float dt) {
            this->do_work(dt);  // CRASH if MyListener is destroyed!
        });
    }

    void do_work(float dt) { /* ... */ }
};
```

**Fix:** Use `ScopedConnection` as a member variable.

```cpp
class MyListener {
public:
    MyListener(Application* app) {
        conn_ = app->signal_update().connect([this](float dt) {
            this->do_work(dt);  // Safe: connection disconnects in destructor
        });
    }

private:
    sig::scoped_connection conn_;  // Disconnects when MyListener is destroyed
    void do_work(float dt) { /* ... */ }
};
```

### 11.6. Blocking Operations in Callbacks

**Problem:** Performing blocking operations (file I/O, network requests, long computations) inside a signal callback blocks the entire game loop.

```cpp
// BAD: blocks the main thread
app->signal_shutdown().connect([]() {
    save_large_file();  // May take seconds, freezes the game
});
```

**Fix:** Offload to a [coroutine](coroutines.md).

```cpp
// GOOD: runs asynchronously
app->signal_shutdown().connect([this]() {
    cr_async([this]() {
        save_large_file();
    });
});
```

---

## 12. Complete Examples

### Example 1: Event Bus Pattern

A simple event bus that routes game events to interested listeners:

```cpp
#include <simulant/simulant.h>

using namespace smlt;

// Define event types
struct PlayerKilledEvent {
    int player_id;
    Vec3 position;
};

struct ScoreChangedEvent {
    int player_id;
    int new_score;
};

class EventBus {
public:
    sig::signal<void(const PlayerKilledEvent&)>& on_player_killed() {
        return on_player_killed_;
    }

    sig::signal<void(const ScoreChangedEvent&)>& on_score_changed() {
        return on_score_changed_;
    }

    void emit_player_killed(int player_id, Vec3 position) {
        on_player_killed_({player_id, position});
    }

    void emit_score_changed(int player_id, int new_score) {
        on_score_changed_({player_id, new_score});
    }

private:
    sig::signal<void(const PlayerKilledEvent&)> on_player_killed_;
    sig::signal<void(const ScoreChangedEvent&)> on_score_changed_;
};

// Usage in a game class
class Game : public Application {
public:
    Game(const AppConfig& config) : Application(config) {}

    bool init() override {
        scenes->register_scene<MainScene>("main");

        // Listen for player death to show a screen effect
        event_bus_.on_player_killed().connect([this](const PlayerKilledEvent& evt) {
            S_INFO("Player {} died at {}", evt.player_id, evt.position);
            show_death_effect(evt.position);
        });

        // Listen for score changes to update HUD
        event_bus_.on_score_changed().connect([this](const ScoreChangedEvent& evt) {
            update_hud_score(evt.player_id, evt.new_score);
        });

        return true;
    }

    EventBus& event_bus() { return event_bus_; }

private:
    EventBus event_bus_;

    void show_death_effect(Vec3 position) { /* ... */ }
    void update_hud_score(int player_id, int new_score) { /* ... */ }
};

// Somewhere in game logic:
// game->event_bus().emit_player_killed(1, player_position);
```

### Example 2: Automatic Health Bar Updates

A health bar widget that automatically updates when an entity's health changes, using signals instead of polling:

```cpp
class HealthBar : public Widget {
public:
    HealthBar(Scene* scene, Entity* entity)
        : Widget(scene, STAGE_NODE_TYPE_WIDGET),
          entity_(entity)
    {
        if (entity_) {
            // Update whenever the entity is updated
            health_conn_ = entity_->signal_update().connect(
                [this](float dt) { update_bar(); }
            );

            // Remove ourselves if the entity is destroyed
            destroy_conn_ = entity_->signal_destroyed().connect([this]() {
                entity_ = nullptr;
                this->destroy();
            });
        }
    }

    ~HealthBar() override {
        // Connections clean up automatically (if using scoped_connection),
        // but explicit disconnect is fine too
    }

private:
    Entity* entity_;
    sig::scoped_connection health_conn_;
    sig::scoped_connection destroy_conn_;

    void update_bar() {
        if (!entity_) return;
        float health_pct = entity_->health() / entity_->max_health();
        set_fill_fraction(health_pct);

        // Change color based on health level
        if (health_pct < 0.25f) {
            set_color(Color::RED);
        } else if (health_pct < 0.5f) {
            set_color(Color::ORANGE);
        } else {
            set_color(Color::GREEN);
        }
    }
};
```

### Example 3: Scene Transition System

Using signals to manage scene loading and transitions:

```cpp
class SceneTransitionManager {
public:
    SceneTransitionManager(Application* app) : app_(app) {
        // Fade in when a scene activates
        app_->scenes()->signal_scene_activated().connect(
            [this](const std::string& route, Scene* scene) {
                fade_in(scene);
            }
        );

        // Fade out when a scene deactivates
        app_->scenes()->signal_scene_deactivated().connect(
            [this](const std::string& route, Scene* scene) {
                fade_out(scene);
            }
        );
    }

    void transition_to(const std::string& scene_name) {
        // Start a coroutine for the transition
        cr_async([this, scene_name]() {
            is_transitioning_ = true;

            // Fade out current scene
            fade_out_current();
            cr_yield_for(0.5f);  // Wait 0.5 seconds

            // Switch scene
            app_->scenes()->set_active_scene(scene_name);
            cr_yield_for(0.1f);  // Let new scene initialize

            // Fade in new scene
            fade_in_current();
            cr_yield_for(0.5f);

            is_transitioning_ = false;
        });
    }

private:
    Application* app_;
    bool is_transitioning_ = false;

    void fade_in(Scene* scene) { /* animate overlay alpha */ }
    void fade_out(Scene* scene) { /* animate overlay alpha */ }
    void fade_out_current() { /* ... */ }
    void fade_in_current() { /* ... */ }
};
```

### Example 4: Physics-Based Trigger Volumes

Using physics signals to implement trigger zones:

```cpp
class TriggerVolume : public StageNode {
public:
    TriggerVolume(Scene* scene, Vec3 size)
        : StageNode(scene, STAGE_NODE_TYPE_COLLIDER)
    {
        // Create a trigger body (non-colliding, but reports contacts)
        body_ = create_trigger_body(size);

        // Track who is inside
        body_->signal_collision_enter().connect(
            [this](const Collision& c) {
                auto other = c.other->get_owner();
                entities_inside_.insert(other->id());
                on_entity_entered(other);
            }
        );

        body_->signal_collision_exit().connect(
            [this](const Collision& c) {
                auto other = c.other->get_owner();
                entities_inside_.erase(other->id());
                on_entity_exited(other);
            }
        );
    }

    sig::signal<void(StageNode*)>& signal_entity_entered() {
        return signal_entity_entered_;
    }

    sig::signal<void(StageNode*)>& signal_entity_exited() {
        return signal_entity_exited_;
    }

    bool contains(StageNodeID id) const {
        return entities_inside_.count(id) > 0;
    }

    const std::unordered_set<StageNodeID>& entities_inside() const {
        return entities_inside_;
    }

protected:
    virtual void on_entity_entered(StageNode* entity) {
        S_INFO("Entity {} entered trigger {}", entity->name(), name());
        signal_entity_entered_(entity);
    }

    virtual void on_entity_exited(StageNode* entity) {
        S_INFO("Entity {} exited trigger {}", entity->name(), name());
        signal_entity_exited_(entity);
    }

private:
    PhysicsBody* body_;
    std::unordered_set<StageNodeID> entities_inside_;
    sig::signal<void(StageNode*)> signal_entity_entered_;
    sig::signal<void(StageNode*)> signal_entity_exited_;
};

// Usage:
auto door_trigger = scene->create_child<TriggerVolume>(Vec3(2, 3, 2));
door_trigger->set_name("door_trigger");
door_trigger->signal_entity_entered().connect(
    [](StageNode* entity) {
        if (entity->name() == "player") {
            S_INFO("Player entered door trigger - open the door!");
        }
    }
);
```

### Example 5: Frame-Rate Independent Timer with Signals

```cpp
class SignalTimer {
public:
    SignalTimer(Application* app) {
        conn_ = app->signal_update().connect([this](float dt) {
            if (!running_) return;

            elapsed_ += dt;
            if (elapsed_ >= interval_) {
                elapsed_ -= interval_;
                on_tick_();
            }
        });
    }

    void start(float interval_seconds) {
        interval_ = interval_seconds;
        elapsed_ = 0.0f;
        running_ = true;
    }

    void stop() {
        running_ = false;
    }

    sig::signal<void()>& on_tick() {
        return on_tick_;
    }

private:
    sig::scoped_connection conn_;
    sig::signal<void()> on_tick_;
    float interval_ = 1.0f;
    float elapsed_ = 0.0f;
    bool running_ = false;
};

// Usage: create a timer that fires every 2 seconds
SignalTimer timer(app);
timer.on_tick().connect([]() {
    S_INFO("Two seconds elapsed!");
});
timer.start(2.0f);
```

---

## Summary

| Concept | Key Point |
|---|---|
| **Signals** | Type-safe event publishers; use `sig::signal<Signature>` |
| **Connections** | Returned by `connect()`. Use `disconnect()` to remove. |
| **ScopedConnection** | RAII connection; disconnects automatically on destruction. **Preferred** in most cases. |
| **connect_once()** | Auto-disconnects after first invocation. |
| **DEFINE_SIGNAL** | Macro for declaring signals as class members with public accessors. |
| **Emission** | Call `signal_(args...)` or `signal_name()(args...)` for macro-defined signals. |
| **Safety** | Always use `ScopedConnection` when `this` is captured to avoid dangling callbacks. |
| **Performance** | Signals use a linked list; emission is O(n). Avoid excessive listeners in hot paths. |
| **Thread safety** | Not thread-safe. Use only from the main thread. |
