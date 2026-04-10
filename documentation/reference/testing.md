# Testing Framework

Simulant includes a built-in unit testing framework designed specifically for game engine testing. It provides fixtures for Application, Window, and Scene objects so you can test engine features without writing boilerplate.

**Related documentation:**

- [Project Structure](../getting-started/project-structure.md)
- [Coroutines](../scripting/coroutines.md)
- [Physics](../physics/overview.md)

---

## 1. Overview

The Simulant testing framework is a lightweight, header-only test runner. It uses:

- **`TestCase`** -- A base class for tests that do not need engine fixtures (math, strings, utility functions).
- **`SimulantTestCase`** -- A subclass of `TestCase` that automatically creates a headless `Application`, `Window`, and `Scene`. Use this for anything that interacts with the engine (nodes, physics, coroutines, materials, etc.).
- **Assertion macros** -- `assert_equal`, `assert_true`, `assert_false`, `assert_close`, `assert_raises`, and more.
- **Automatic test discovery** -- A Python script (`tools/test_generator.py`) scans your test headers for classes that inherit from `TestCase` or `SimulantTestCase` and generates a `main.cpp` that registers every `test_*` method.

Tests are compiled into a single `simulant_tests` executable. You run all tests, a subset, or a single test by passing a class name prefix.

---

## 2. Test File Structure and Organisation

Test files live in the `tests/` directory of your project. By convention each file is named `test_<feature>.h` and contains one or more test case classes.

```
tests/
├── test_coroutines.h      # Coroutine-related tests
├── test_physics.h         # Physics/collider tests
├── test_scene.h           # Scene and SceneManager tests
├── test_object.h          # StageNode / transform tests
├── test_math.h            # Pure math tests (no fixtures)
└── ...
```

The `tests/CMakeLists.txt` glob collects all `*.h` files and passes them to the test generator, which produces a `main.cpp` in the build directory. You rarely need to touch `main.cpp` directly -- it is regenerated automatically whenever test headers change.

### Naming conventions

- **File:** `test_<lowercase_feature>.h` (e.g. `test_physics.h`, `test_coroutines.h`)
- **Class:** `<FeatureName>Tests` or `<FeatureName>Test` (e.g. `ColliderTests`, `MathTest`)
- **Method:** `test_<lowercase_description>` (e.g. `test_create_kinematic_body`)

---

## 3. SimulantTestCase Base Class

`SimulantTestCase` is defined in `simulant/test.h`. It extends `TestCase` and provides three ready-to-use protected members that are initialised before every test:

| Member | Type | Description |
|--------|------|-------------|
| `application` | `std::shared_ptr<Application>` | The headless application instance |
| `window` | `Window*` | The test window (640x480, windowed) |
| `scene` | `Scene*` | The active scene ("main") |

### Lifecycle

1. **`set_up()`** -- Called before each test method. `SimulantTestCase::set_up()` creates the `Application` (once) and resets the scene for every subsequent test.
2. **Your test method runs.**
3. **`tear_down()`** -- Called after each test method, even if the test threw an exception.

Always call the parent implementation when you override these methods:

```cpp
class MyTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();   // Required
        // Your additional setup...
    }

    void tear_down() {
        // Your additional cleanup...
        SimulantTestCase::tear_down();  // Required
    }
};
```

### Reusing the Application

The `Application` is created **once** and reused across all tests in a test run. Between tests the scene is unloaded and reloaded, and all coroutines are stopped. This keeps test startup fast while ensuring test isolation.

---

## 4. TestApp and TestScene for Fixtures

When `SimulantTestCase` creates the application it uses two internal classes:

### TestApp

A minimal `Application` subclass that registers a `TestScene` under the name `"main"`. The app config sets up:

- 640x480 windowed mode
- `LOG_LEVEL_WARN` log level
- Search paths: `assets/` and `sample_data/`

### TestScene

A minimal `Scene` subclass whose `on_load()` is empty. It serves as a clean slate for every test.

If you need custom scene behaviour in your tests, simply create your own scene class and use `scene` methods to populate it:

```cpp
void test_my_feature() {
    auto stage = scene->create_child<smlt::Stage>();
    auto camera = scene->create_child<smlt::Camera3D>();
    // ... test logic
}
```

---

## 5. Assertion Macros

All assertion macros are defined in `simulant/test.h` and are available in any class that inherits from `TestCase` or `SimulantTestCase`.

| Macro | Purpose | Example |
|-------|---------|---------|
| `assert_equal(expected, actual)` | Fail if `expected != actual` | `assert_equal(5, result);` |
| `assert_not_equal(lhs, rhs)` | Fail if values are equal | `assert_not_equal(id1, id2);` |
| `assert_true(value)` | Fail if value is falsy | `assert_true(node);` |
| `assert_false(value)` | Fail if value is truthy | `assert_false(is_active);` |
| `assert_close(expected, actual, tolerance)` | Fail if values differ by more than `tolerance` | `assert_close(1.0f, result, 0.001f);` |
| `assert_is_null(ptr)` | Fail if pointer is not null | `assert_is_null(node);` |
| `assert_is_not_null(ptr)` | Fail if pointer is null | `assert_is_not_null(hit);` |
| `assert_raises(ExceptionType, lambda)` | Fail if the lambda does not throw `ExceptionType` | `assert_raises(std::logic_error, [&]() { risky_call(); });` |
| `assert_items_equal(containerA, containerB)` | Fail if containers have different sizes or missing items | `assert_items_equal(found, expected);` |
| `not_implemented()` | Mark a test as not yet implemented (shows as SKIPPED) | `not_implemented();` |

### Usage examples

```cpp
void test_equality() {
    auto node = scene->create_child<smlt::Stage>();
    assert_equal(0, node->child_count());
    assert_true(node->is_visible());
    assert_is_not_null(node->transform);
}

void test_close_enough() {
    float result = 1.0f / 3.0f;
    assert_close(0.333333f, result, 0.00001f);
}

void test_exception() {
    assert_raises(std::logic_error, [&]() {
        manager->activate("nonexistent");
    });
}

void test_container() {
    std::vector<int> found = {1, 2, 3};
    std::vector<int> expected = {3, 1, 2};  // order does not matter
    assert_items_equal(found, expected);
}
```

---

## 6. Writing Your First Test

Here is the minimum test that uses engine fixtures:

```cpp
// tests/test_hello.h
#pragma once

#include "simulant/test.h"

namespace {
using namespace smlt;

class HelloTests : public smlt::test::SimulantTestCase {
public:
    void test_scene_exists() {
        // scene and application are provided by SimulantTestCase
        assert_is_not_null(scene);
        assert_is_not_null(window);
        assert_is_not_null(application);
    }

    void test_create_node() {
        auto node = scene->create_child<smlt::Stage>();
        assert_is_not_null(node);
        assert_equal(0u, node->child_count());
    }
};

}
```

That is it. The test generator will find `HelloTests`, discover both `test_*` methods, and register them. Rebuild and run:

```bash
simulant build
simulant test HelloTests
```

### Tests that do not need engine fixtures

If you are testing pure logic (math, string utils, etc.), inherit from `smlt::test::TestCase` instead. This avoids the overhead of creating an Application:

```cpp
// tests/test_math.h
#pragma once

#include "simulant/test.h"

namespace {
using namespace smlt;

class MathTest : public smlt::test::TestCase {
public:
    void test_lerp() {
        assert_equal(0.0f, smlt::lerp(100.0f, -100.0f, 0.5f));
        assert_equal(50.0f, smlt::lerp(-100.0f, 100.0f, 0.75f));
    }
};

}
```

---

## 7. Testing Scenes and StageNodes

### Creating and verifying nodes

```cpp
void test_create_geom() {
    auto mesh = scene->assets->create_mesh(smlt::VertexSpecification::DEFAULT);
    mesh->create_submesh_as_cube("cube", scene->assets->create_material(), 1.0f);

    auto geom = scene->create_child<smlt::Geom>(mesh);
    assert_equal(STAGE_NODE_TYPE_GEOM, geom->node_type());
}
```

### Testing parent-child relationships

```cpp
void test_child_inherits_transform() {
    auto parent = scene->create_child<smlt::Stage>();
    auto child  = scene->create_child<smlt::Stage>();
    child->set_parent(parent);

    parent->transform->set_translation(Vec3(0, 10, 0));
    assert_equal(Vec3(0, 10, 0), child->transform->position());
    assert_equal(Vec3(), child->transform->translation());  // relative is zero
}
```

### Testing deferred destruction

Nodes are not destroyed immediately when you call `destroy()`. They are cleaned up at the end of the next frame. Use `application->run_frame()` to trigger cleanup:

```cpp
void test_node_destruction() {
    auto count = scene->count_nodes_by_type<smlt::Stage>();
    auto node = scene->create_child<smlt::Stage>();

    assert_equal(count + 1, scene->count_nodes_by_type<smlt::Stage>(true));

    node->destroy();
    assert_true(node->is_destroyed());

    // Node is still counted until the frame ends
    assert_equal(count + 1, scene->count_nodes_by_type<smlt::Stage>(true));

    application->run_frame();

    // Now it is gone
    assert_equal(count, scene->count_nodes_by_type<smlt::Stage>(true));
}
```

### Iterating the scene graph

```cpp
void test_descendent_iteration() {
    auto root = scene->create_child<smlt::Stage>();
    auto a = scene->create_child<smlt::Stage>();
    auto b = scene->create_child<smlt::Stage>();
    a->set_parent(root);
    b->set_parent(root);

    std::set<smlt::StageNode*> found;
    for (auto& node : root->each_child()) {
        found.insert(&node);
    }

    assert_items_equal(found, std::set<smlt::StageNode*>{a, b});
}
```

---

## 8. Testing Physics

Physics tests require the `PhysicsService` to be started on the scene. A common pattern is to start it in `set_up()` and stop it in `tear_down()`:

```cpp
class ColliderTests : public smlt::test::SimulantTestCase {
private:
    PhysicsService* physics;

public:
    void set_up() {
        SimulantTestCase::set_up();
        physics = scene->start_service<PhysicsService>();
        physics->set_gravity(Vec3());  // disable gravity for predictable tests
    }

    void tear_down() {
        scene->stop_service<PhysicsService>();
        SimulantTestCase::tear_down();
    }

    void test_box_collider() {
        auto body = scene->create_child<DynamicBody>();
        body->add_box_collider(Vec3(2, 2, 1), PhysicsMaterial::wood());

        auto hit = physics->ray_cast(Vec3(0, 2, 0), Vec3(0, -1, 0), 2);
        assert_true(hit);
        assert_close(1.0f, hit->distance, 0.0001f);
    }
};
```

### Collision listeners

Implement `CollisionListener` and register it on a body:

```cpp
class TestListener : public CollisionListener {
public:
    bool enter_called = false;
    bool leave_called = false;

    void on_collision_enter(const Collision& c) override { enter_called = true; }
    void on_collision_exit(const Collision& c) override  { leave_called = true; }
    void on_collision_stay() override {}
};

void test_collision_events() {
    TestListener listener;

    auto body_a = scene->create_child<StaticBody>();
    body_a->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::wood());
    body_a->register_collision_listener(&listener);

    auto body_b = scene->create_child<DynamicBody>();
    body_b->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::wood());

    // Step physics
    physics->fixed_update(1.0f / 60.0f);

    assert_true(listener.enter_called);
    assert_false(listener.leave_called);
}
```

See [Physics Overview](../physics/overview.md) for details on body types, colliders, and joints.

---

## 9. Testing Coroutines

Coroutines are tested using `cr_await`, `cr_async`, and `cr_yield`. You can test them synchronously in a single test method, or spread across multiple frames.

### Synchronous coroutine test

```cpp
void test_await() {
    auto value = cr_await(
        cr_async([]() -> int {
            int j = 0;
            for (int i = 0; i < 100; ++i) {
                j++;
                cr_yield();
            }
            return j;
        })
    );

    assert_equal(value, 100);
}
```

### Testing time-based coroutines

Use `cr_yield_for()` and manually advance time:

```cpp
void test_yield_for() {
    bool called = false;

    auto ret = cr_async([&]() -> bool {
        cr_yield_for(Seconds(0.1f));
        called = true;
        return true;
    });

    application->update_coroutines();
    assert_false(called);                     // not ready yet

    thread::sleep(200);                        // wait real time
    application->update_coroutines();
    assert_true(called);                       // now ready
}
```

### Testing coroutine ordering

```cpp
void test_coroutine_order() {
    int counter = 3;
    std::vector<int> order;
    bool done = false;

    std::function<void(int)> cb = [&](int a) {
        if (a < 3) {
            cr_async([&]() { cb(a + 1); });
        }
        while (!done) {
            order.push_back(a);
            ++counter;
            cr_yield();
        }
    };

    cr_async([&]() { cb(1); });

    application->run_frame();
    application->run_frame();
    application->run_frame();

    assert_equal(order.size(), 9u);
    assert_equal(order[0], 1);
    assert_equal(order[1], 2);
    assert_equal(order[2], 3);

    done = true;
    application->stop_all_coroutines();
}
```

### Connecting to application signals

```cpp
void test_runs_on_update() {
    int update_count = 0;
    auto sig = application->signal_update().connect([&](float) {
        update_count++;
    });

    application->run_frame();
    assert_equal(1, update_count);

    sig.disconnect();
}
```

See [Coroutines](../scripting/coroutines.md) for the full coroutine API.

---

## 10. Running Tests

### Building tests

Tests are built alongside your project when `SIMULANT_BUILD_TESTS` is enabled (on by default for most platforms):

```bash
simulant build
```

### Running all tests

```bash
simulant test
```

This runs the `simulant_tests` executable. The CLI tool passes the working directory so that asset paths (`assets/`, `sample_data/`) resolve correctly.

### Running a specific test class

Pass a class name prefix to filter tests:

```bash
simulant test CoroutineTests
simulant test ColliderTests
```

Only tests whose fully-qualified name starts with the given string will run.

### Running a single test method

```bash
simulant test "CoroutineTests::test_await"
```

### JUnit XML output

Generate a JUnit-compatible XML report for CI integration:

```bash
simulant test --junit-xml=report.xml
```

---

## 11. Test Output and Debugging

### Coloured output

The test runner uses ANSI colour codes:

| Colour | Status |
|--------|--------|
| Green (`OK`) | Test passed |
| Yellow (`FAILED`) | Assertion failed |
| Red (`EXCEPT`) | Unexpected exception thrown |
| Blue (`SKIPPED`) | Test was skipped (`not_implemented()` or `skip_if`) |

### Failure output

When a test fails, the runner prints:

1. The assertion message
2. The file and line number
3. The source line that failed

```
    CoroutineTests::test_yield_and_wait                    FAILED
        false is not true
        /home/user/project/tests/test_coroutines.h:67
        assert_false(called);
```

### Debugging a failing test

1. **Run the specific test** to isolate the failure:
   ```bash
   simulant test "MyTests::test_something"
   ```

2. **Check asset paths.** Tests expect `assets/` and `sample_data/` relative to the working directory. If a test loads assets and fails, verify the paths exist.

3. **Use `LOG_LEVEL_WARN` or lower.** The test app defaults to `LOG_LEVEL_WARN`. You can change the config in `test.h` if you need more output.

4. **Step through in a debugger.** The test executable is a normal binary. Attach GDB or your IDE debugger and set breakpoints on your test methods.

---

## 12. Skipping Tests

### Skipping with `not_implemented()`

For tests you have not written the body of yet:

```cpp
void test_future_feature() {
    not_implemented();
}
```

This displays the test as **SKIPPED** in blue and does not count as a failure.

### Conditional skipping with `skip_if()`

Skip a test at runtime based on a condition:

```cpp
void test_platform_specific_feature() {
    skip_if(!has_opengl(), "Requires OpenGL");
    // ... test code
}
```

The reason string is displayed in the test output:

```
    MyTests::test_platform_specific_feature                SKIPPED
```

### Known issues

```cpp
void test_collision_stay() {
    skip_if(true, "Not yet implemented");
    // ... test code
}
```

This is useful for documenting known gaps in test coverage.

---

## 13. Test Best Practices

### Keep tests isolated

Each test method should test **one thing**. Do not chain multiple assertions about different features in one method.

```cpp
// Good
void test_node_position() { ... }
void test_node_rotation() { ... }

// Bad
void test_node_everything() { ... }
```

### Reset state in set_up / tear_down

If you start services, register signals, or create persistent state, clean it up:

```cpp
void set_up() {
    SimulantTestCase::set_up();
    physics = scene->start_service<PhysicsService>();
}

void tear_down() {
    scene->stop_service<PhysicsService>();
    SimulantTestCase::tear_down();
}
```

### Disconnect signals

Use `sig::scoped_connection` so signals disconnect automatically when the connection object goes out of scope:

```cpp
void test_signal() {
    bool called = false;
    sig::scoped_connection conn = node->signal_destroyed().connect([&]() {
        called = true;
    });
    // conn disconnects automatically when function returns
}
```

### Use `application->run_frame()` for deferred operations

Many engine operations are deferred (node destruction, pipeline cleanup, scene transitions). Call `application->run_frame()` to flush the deferred queue:

```cpp
node->destroy();
application->run_frame();   // actual destruction happens here
assert_equal(0, scene->count_nodes_by_type<smlt::Stage>(true));
```

### Stop coroutines

Always call `application->stop_all_coroutines()` at the end of tests that spawn coroutines, especially tests that use infinite loops:

```cpp
void test_my_coroutine() {
    bool done = false;
    cr_async([&]() {
        while (!done) {
            cr_yield();
        }
    });

    // ... assertions

    done = true;
    application->stop_all_coroutines();
}
```

### Prefer `SimulantTestCase` when touching the engine

If your test creates `Stage` nodes, `Geom`s, materials, or anything that needs a `Scene`, use `SimulantTestCase`. Use plain `TestCase` only for pure utility logic.

---

## 14. Common Patterns in Existing Tests

### Pattern: Physics service with gravity disabled

```cpp
void set_up() {
    SimulantTestCase::set_up();
    physics = scene->start_service<PhysicsService>();
    physics->set_gravity(Vec3());  // zero gravity for deterministic tests
}
```

Found in: `ColliderTests`, `DynamicBodyTest`

### Pattern: Scene with custom scene class for lifecycle testing

```cpp
class TestScene : public Scene {
public:
    TestScene(Window* window) : Scene(window) {}
    void on_load() override { load_called = true; }
    void on_unload() override { unload_called = true; }
    bool load_called = false;
    bool unload_called = false;
};

void test_activate() {
    manager_->register_scene<TestScene>("main");
    TestScene* scr = dynamic_cast<TestScene*>(manager_->resolve_scene("main").get());
    manager_->activate("main");
    manager_->late_update(1.0f);
    assert_true(scr->load_called);
}
```

Found in: `SceneManagerTests`

### Pattern: Manual SceneManager lifecycle

```cpp
class SceneManagerTests : public smlt::test::SimulantTestCase {
private:
    SceneManager::ptr manager_;

public:
    void set_up() {
        SimulantTestCase::set_up();
        manager_ = std::make_shared<SceneManager>(window);
        manager_->init();
    }

    void tear_down() {
        manager_->clean_up();
        manager_.reset();
    }
};
```

### Pattern: Node counting for cleanup verification

```cpp
void test_nodes_freed() {
    auto count = scene->count_nodes_by_type<smlt::Stage>();
    auto node = scene->create_child<smlt::Stage>();

    assert_equal(count + 1, scene->count_nodes_by_type<smlt::Stage>(true));

    node->destroy();
    application->run_frame();

    assert_equal(count, scene->count_nodes_by_type<smlt::Stage>(true));
}
```

Found in: `SceneTests` (for lights, cameras, geoms, particle systems, sprites, etc.)

### Pattern: CollisionListener as a local helper class

```cpp
class Listener : public CollisionListener {
public:
    Listener(bool* enter, uint32_t* stay, bool* leave)
        : enter_called(enter), stay_count(stay), leave_called(leave) {}

    void on_collision_enter(const Collision& c) override { if(enter_called) *enter_called = true; }
    void on_collision_stay() override { if(stay_count) *stay_count += 1; }
    void on_collision_exit(const Collision& c) override { if(leave_called) *leave_called = true; }

    bool* enter_called = nullptr;
    uint32_t* stay_count = nullptr;
    bool* leave_called = nullptr;
};
```

### Pattern: Anonymous namespace

All test files wrap their classes in an anonymous namespace to avoid link-time conflicts when the generated `main.cpp` includes all test headers:

```cpp
#pragma once
#include "simulant/test.h"

namespace {
using namespace smlt;

class MyTests : public smlt::test::SimulantTestCase {
    // ...
};

}
```

---

## 15. Complete Test Example

This example combines multiple patterns into a single comprehensive test file:

```cpp
// tests/test_my_feature.h
#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {
using namespace smlt;

// --- Helper class for testing collision events ---
class TestCollisionListener : public CollisionListener {
public:
    int enter_count = 0;
    int stay_count = 0;
    int exit_count = 0;

    void on_collision_enter(const Collision&) override { enter_count++; }
    void on_collision_stay() override                  { stay_count++; }
    void on_collision_exit(const Collision&) override  { exit_count++; }
};

// --- Main test class ---
class MyFeatureTests : public smlt::test::SimulantTestCase {
private:
    PhysicsService* physics = nullptr;

public:
    // --- Lifecycle ---

    void set_up() {
        SimulantTestCase::set_up();

        // Start physics with no gravity
        physics = scene->start_service<PhysicsService>();
        physics->set_gravity(Vec3());
    }

    void tear_down() {
        scene->stop_service<PhysicsService>();
        SimulantTestCase::tear_down();
    }

    // --- Tests ---

    void test_node_creation() {
        auto stage = scene->create_child<smlt::Stage>();
        stage->set_name("my_stage");

        assert_is_not_null(stage);
        assert_equal(STAGE_NODE_TYPE_STAGE, stage->node_type());
        assert_equal("my_stage", stage->name());
    }

    void test_parent_child_transform() {
        auto parent = scene->create_child<smlt::Stage>();
        auto child  = scene->create_child<smlt::Stage>();
        child->set_parent(parent);

        parent->transform->set_translation(Vec3(10, 0, 0));
        parent->transform->set_rotation(
            Quaternion(Vec3::up(), Degrees(90))
        );

        // World position follows parent
        assert_close(10.0f, child->transform->position().x, 0.0001f);

        // Local position is still zero
        assert_equal(Vec3(), child->transform->translation());
    }

    void test_physics_raycast() {
        auto body = scene->create_child<DynamicBody>();
        body->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::wood());

        auto hit = physics->ray_cast(Vec3(0, 5, 0), Vec3(0, -1, 0), 10);

        assert_true(hit);
        assert_close(4.0f, hit->distance, 0.001f);
    }

    void test_collision_events() {
        TestCollisionListener listener;

        auto body_a = scene->create_child<StaticBody>();
        body_a->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::wood());
        body_a->register_collision_listener(&listener);

        auto body_b = scene->create_child<DynamicBody>();
        body_b->add_box_collider(Vec3(1, 1, 1), PhysicsMaterial::wood());

        // Single physics step triggers enter
        physics->fixed_update(1.0f / 60.0f);
        assert_equal(1, listener.enter_count);
        assert_equal(0, listener.exit_count);
    }

    void test_coroutine_runs() {
        int counter = 0;

        auto promise = cr_async([&]() -> int {
            for (int i = 0; i < 10; ++i) {
                counter++;
                cr_yield();
            }
            return counter;
        });

        int result = cr_await(promise);
        assert_equal(10, result);
        assert_equal(10, counter);
    }

    void test_deferred_destruction() {
        auto count = scene->count_nodes_by_type<smlt::Stage>();
        auto node = scene->create_child<smlt::Stage>();

        node->destroy();
        assert_true(node->is_destroyed());

        // Still counted until frame end
        assert_equal(count + 1, scene->count_nodes_by_type<smlt::Stage>(true));

        application->run_frame();

        // Now removed
        assert_equal(count, scene->count_nodes_by_type<smlt::Stage>(true));
    }

    void test_node_hierarchy_cleanup() {
        auto root = scene->create_child<smlt::Stage>();
        auto child = scene->create_child<smlt::Stage>();
        child->set_parent(root);

        // Destroying the parent cascades to children
        root->destroy();
        application->run_frame();

        assert_true(root->is_destroyed());
        assert_true(child->is_destroyed());
    }

    void test_signal_connection() {
        bool destroyed = false;

        auto node = scene->create_child<smlt::Stage>();
        sig::scoped_connection conn = node->signal_destroyed().connect([&]() {
            destroyed = true;
        });

        node->destroy();
        assert_true(destroyed);

        // conn auto-disconnects when it goes out of scope
    }

    void test_not_yet_ready() {
        skip_if(true, "Waiting for feature X to be merged");
        // Future test code here
    }

    void test_placeholder() {
        not_implemented();
    }
};

} // anonymous namespace
```

---

## Quick Reference Card

```cpp
// Inherit from this for engine tests
class MyTests : public smlt::test::SimulantTestCase { ... };

// Inherit from this for pure-logic tests
class MyTests : public smlt::test::TestCase { ... };

// Assertions
assert_equal(expected, actual)
assert_not_equal(a, b)
assert_true(x)
assert_false(x)
assert_close(expected, actual, tolerance)
assert_is_null(ptr)
assert_is_not_null(ptr)
assert_raises(ExceptionType, lambda)
assert_items_equal(containerA, containerB)
not_implemented()
skip_if(condition, reason)

// Fixture members (SimulantTestCase)
application     // std::shared_ptr<Application>
window          // Window*
scene           // Scene*

// Running
simulant test                        // all tests
simulant test CoroutineTests         // by class
simulant test "Class::method"        // single test
simulant test --junit-xml=report.xml // JUnit output
```
