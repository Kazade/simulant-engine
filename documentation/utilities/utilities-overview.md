# Utilities Overview

Simulant ships with a rich set of utility classes and functions covering math, logging, random numbers, JSON parsing, profiling, timing, and more. This guide walks through each utility with practical examples.

---

## 1. Math Library

All math types live in the `smlt` namespace. Source files are in `simulant/math/`.

### Vec2, Vec3, Vec4

These are the core vector types. Each supports arithmetic operators, normalization, dot/cross products, and more.

```cpp
#include <simulant/math/vec2.h>
#include <simulant/math/vec3.h>
#include <simulant/math/vec4.h>

// Construction
Vec2 v2(1.0f, 2.0f);
Vec3 v3(1.0f, 2.0f, 3.0f);
Vec4 v4(1.0f, 2.0f, 3.0f, 1.0f);

// Static factories
Vec3 origin = Vec3::zero();
Vec3 all_ones = Vec3::one();
Vec3 up = Vec3::up();       // (0, 1, 0)
Vec3 forward = Vec3::forward(); // (0, 0, -1)  // OpenGL convention

// Arithmetic
Vec3 a(1, 2, 3);
Vec3 b(4, 5, 6);
Vec3 sum = a + b;
Vec3 diff = a - b;
Vec3 scaled = a * 2.0f;
Vec3 div = a / 2.0f;

// Length and normalization
float len = a.length();
float lenSq = a.length_squared();
Vec3 norm = a.normalized();   // returns normalized copy
a.normalize();                 // normalizes in-place

// Dot and cross
float dot = a.dot(b);
Vec3 cross = a.cross(b);

// Lerp (linear interpolation)
Vec3 result = a.lerp(b, 0.5f);  // halfway between a and b

// Smooth lerp (frame-rate independent)
// dt = delta time, p = target precision (e.g. 0.01), t = expected duration
Vec3 smooth = a.lerp_smooth(b, dt, 0.01f, 1.0f);

// Exponential decay lerp (Freya Holmer style)
Vec3 decayed = a.lerp_decay(b, dt, 5.0f);

// Projection and components
Vec3 projected = a.project_onto_vec3(b.normalized());
Vec3 parallel = a.parallel_component(unit_basis);
Vec3 perpendicular = a.perpendicular_component(unit_basis);

// Rotation and transformation
Vec3 rotated = v3.rotated_by(quaternion);
Vec3 transformed = v3.transformed_by(mat4);

// Conversion between types
Vec2 xy = v3.xy();
Vec4 with_w = v3.xyzw(1.0f);
Vec3 from_v2 = v2.xyz(0.0f);
Vec3 from_v4 = v4.xyz();

// Distance
float dist = a.distance_to(b);
float sqDist = a.squared_distance_to(b);

// Clamping magnitude
a.limit(5.0f);  // caps length to 5.0

// Average of a collection
std::vector<Vec3> vectors = { ... };
Vec3 avg = Vec3::find_average(vectors);

// Min/max per-component
Vec3 minv = Vec3::min(a, b);
Vec3 maxv = Vec3::max(a, b);
```

### Quaternion

Quaternions represent 3D rotations without gimbal lock. They are the preferred way to store and compose rotations.

```cpp
#include <simulant/math/quaternion.h>

// Identity quaternion (no rotation)
Quaternion identity;

// From Euler angles (pitch, yaw, roll in Degrees)
Quaternion q(Degrees(90), Degrees(45), Degrees(0));

// From axis and angle
Quaternion axis_angle(Vec3(0, 1, 0), Degrees(90));

// From Euler struct
Quaternion from_euler(Euler(90, 45, 0));

// From a rotation matrix
Quaternion from_mat3(Mat3 rot);

// Extract Euler angles
Euler euler = q.to_euler();
Degrees pitch = q.pitch();
Degrees yaw = q.yaw();
Degrees roll = q.roll();

// Axis-angle extraction
AxisAngle aa = q.to_axis_angle();
Vec3 axis = q.axis();
Radians angle = q.angle();

// Composition (multiply rotations)
Quaternion combined = q1 * q2;

// Slerp (spherical linear interpolation)
Quaternion result = q1.slerp(q2, 0.5f);

// Nlerp (normalized linear interpolation -- faster, slightly less accurate)
Quaternion nresult = q1.nlerp(q2, 0.5f);

// Inverse
Quaternion inv = q.inversed();

// Direction vectors from a quaternion
Vec3 fwd = q.forward();
Vec3 up_dir = q.up();
Vec3 right_dir = q.right();

// Look-at rotation (rotates to face a direction)
Quaternion look = Quaternion::look_rotation(target_dir, Vec3(0, 1, 0));
```

### Mat3 and Mat4

Mat3 is a 3x3 rotation matrix. Mat4 is a 4x4 transform matrix (rotation + translation + scale).

```cpp
#include <simulant/math/mat3.h>
#include <simulant/math/mat4.h>

// --- Mat4 ---

// Identity
Mat4 identity;

// Static factories
Mat4 rotX = Mat4::as_rotation_x(Degrees(45));
Mat4 rotY = Mat4::as_rotation_y(Degrees(90));
Mat4 rotZ = Mat4::as_rotation_z(Degrees(180));
Mat4 rotXYZ = Mat4::as_rotation_xyz(Degrees(45), Degrees(90), Degrees(0));

Mat4 lookAt = Mat4::as_look_at(eye_position, target_position, Vec3(0, 1, 0));

Mat4 translation = Mat4::as_translation(Vec3(1, 2, 3));
Mat4 rotation = Mat4::as_rotation(quaternion);
Mat4 scale = Mat4::as_scale(Vec3(2, 2, 2));

// Combined transform
Mat4 transform = Mat4::as_transform(translation_vec, rotation_quat, scale_vec);

// Projection matrices
Mat4 projection = Mat4::as_projection(fov_degrees, aspect_ratio, near_plane, far_plane);
Mat4 ortho = Mat4::as_orthographic(left, right, bottom, top, zNear, zFar);

// Operations
Mat4 inversed = m.inversed();
m.inverse();              // in-place
Mat4 transposed = m.transposed();
m.transpose();            // in-place

Mat4 product = m1 * m2;

// Extract plane from frustum
Plane left_plane = m.extract_plane(FRUSTUM_PLANE_LEFT);

// Access raw data for GPU upload
float* data = m.data();

// --- Mat3 ---

Mat3 rot3 = Mat3::as_rotation_x(Degrees(45));
Mat3 from_quat = Mat3::as_rotation(quaternion);

Vec3 transformed = m3.transform_vector(v3);

Mat3 inv3 = m3.inversed();
Mat3 trans3 = m3.transposed();
```

### Ray

Rays are used for picking and collision queries.

```cpp
#include <simulant/math/ray.h>

Ray ray(origin, direction);   // direction should be normalized

// Access
Vec3 start = ray.start;
Vec3 dir = ray.dir;
Vec3 dir_inv = ray.dir_inv;  // precomputed inverse direction

// Intersection tests
bool hit_aabb = ray.intersects_aabb(aabb);

Vec3 intersection, normal;
float distance;
bool hit_tri = ray.intersects_triangle(v1, v2, v3,
                                       &intersection, &normal, &distance);

bool hit_sphere = ray.intersects_sphere(center, radius,
                                        &intersection, &normal, &distance);
```

### AABB (Axis-Aligned Bounding Box)

AABBs are used for broad-phase collision detection and frustum culling.

```cpp
#include <simulant/math/aabb.h>

// Construction
AABB box;                                  // default (zero)
AABB zero = AABB::zero();
AABB from_center_extent(Vec3(0, 0, 0), Vec3(1, 1, 1));
AABB from_center_size(Vec3(0, 0, 0), 2.0f);
AABB from_dims(Vec3(0, 0, 0), 2.0f, 4.0f, 2.0f);
AABB from_vertices(vertices, count);

// Access
Vec3 center = box.center();
Vec3 min = box.min();
Vec3 max = box.max();
float width = box.width();
float height = box.height();
float depth = box.depth();
Vec3 dims = box.dimensions();

// Containment
bool contains = box.contains_point(Vec3(0.5f, 0.5f, 0.5f));

// Intersection
bool overlap = box.intersects_aabb(other_box);
bool hit_sphere = box.intersects_sphere(center, radius);

// Growing
box.encapsulate(other_box);
box.encapsulate(point);

// Closest point on box to a point
Vec3 closest = box.closest_point(outside_point);

// Corners
std::array<Vec3, 8> corners = box.corners();
```

### Plane

Planes represent infinite flat surfaces, useful for frustum culling and spatial queries.

```cpp
#include <simulant/math/plane.h>

// From normal and distance from origin
Plane plane(normal, distance);

// From normal and a point on the plane
Plane plane2(normal, point_on_plane);

// From explicit coefficients (Ax + By + Cz + D = 0)
Plane plane3(A, B, C, D);

// Distance from point to plane
float dist = plane.distance_to(point);

// Classify point relative to plane
PlaneClassification cls = plane.classify_point(point);
// Returns: PLANE_CLASSIFICATION_IS_BEHIND_PLANE,
//          PLANE_CLASSIFICATION_IS_ON_PLANE,
//          PLANE_CLASSIFICATION_IS_IN_FRONT_OF_PLANE

// Intersect three planes to find a point
optional<Vec3> intersection = Plane::intersect_planes(p1, p2, p3);

// Project a point onto the plane
Vec3 projected = plane.project(point);
```

### Euler, Degrees, and Radians

Simulant uses type-safe angle types to avoid mixing degrees and radians by accident.

```cpp
#include <simulant/math/degrees.h>
#include <simulant/math/radians.h>
#include <simulant/math/euler.h>

// Degrees
Degrees d(90.0f);
float as_float = d.to_float();   // 90.0
Radians r = d.to_radians();

// Radians
Radians rad(1.57f);
float as_float2 = rad.to_float();
Degrees d2 = rad.to_degrees();

// User-defined literals (convenient shorthand)
Degrees angle = 45.0_deg;
Radians angle2 = 3.14159_rad;

// Arithmetic
Degrees doubled = d * 2.0f;
d *= 0.5f;

// Comparison (with angular equivalence)
// -90 degrees is "effectively equal" to 270 degrees
bool eq = d1.is_effectively_equal_to(d2, 0.1f);

// Euler angles (stores three Degree values)
Euler euler(90.0f, 45.0f, 0.0f);
Degrees pitch = euler.x;
Degrees yaw = euler.y;
Degrees roll = euler.z;

// Conversion from Quaternion
Quaternion q(Degrees(30), Degrees(60), Degrees(0));
Euler e = q.to_euler();
```

### Math Utility Constants and Functions

Additional constants and helper functions are in `simulant/math/utils.h`.

```cpp
#include <simulant/math/utils.h>

// Constants
float pi = smlt::PI;           // 3.14159...
float two_pi = smlt::TWO_PI;   // 2 * PI
float eps = smlt::EPSILON;     // machine epsilon for float

// Almost-equal comparison (floating point safe)
bool eq = almost_equal(1.0000001f, 1.0000002f);
bool leq = almost_lequal(a, b, epsilon);
bool geq = almost_gequal(a, b, epsilon);

// Smooth interpolation
float t = smoothstep(edge0, edge1, x);   // Hermite interpolation
float t2 = smootherstep(edge0, edge1, x); // Ken Perlin's improved version

// Clamp
float clamped = clamp(value, min_val, max_val);

// Next power of two (useful for texture dimensions)
uint32_t pot = next_power_of_two(100);  // returns 128

// Sin/cos together (may use hardware sincos on Dreamcast)
float s, c;
fast_sincos(angle, &s, &c);

// Square root, inverse square root
float sq = fast_sqrt(n);
float isq = fast_inverse_sqrt(n);

// Fused multiply-add
float fma = fast_fmaf(a, b, c);  // (a * b) + c
```

---

## 2. Testing Equality: Approximate vs Exact

Floating-point math makes exact equality unreliable. Simulant provides two mechanisms for comparing values:

### `operator==` (Approximate)

The `==` operator on vector and quaternion types uses approximate comparison with `EPSILON` tolerance.

```cpp
Vec3 a(1.0f, 2.0f, 3.0f);
Vec3 b(1.0f + 0.0000001f, 2.0f, 3.0f);

a == b;  // true -- within EPSILON tolerance
```

For Quaternions, `==` checks if the dot product is close to 1.0 (meaning the quaternions represent essentially the same rotation, even if the raw values differ slightly).

```cpp
Quaternion q1(Degrees(90), Degrees(0), Degrees(0));
Quaternion q2(Degrees(90.0001f), Degrees(0), Degrees(0));

q1 == q2;  // true -- approximately equal rotation
```

### `.equals()` (Exact)

When you truly need exact bitwise equality, use the `.equals()` method:

```cpp
Vec3 a(1.0f, 2.0f, 3.0f);
Vec3 b(1.0f, 2.0f, 3.0f);

a.equals(b);  // true -- exact component match

Vec3 c(1.0f, 2.0f, 3.0000001f);
a.equals(c);  // false -- not exactly equal
```

This applies to `Vec2`, `Vec3`, `Vec4`, and `Quaternion`.

### Generic `almost_equal`

For arbitrary floating-point comparisons:

```cpp
#include <simulant/math/utils.h>

float a = 0.1f + 0.2f;
float b = 0.3f;

almost_equal(a, b);  // true
```

---

## 3. Random Numbers

Source: `simulant/utils/random.h`

### RandomGenerator

`RandomGenerator` is a singleton class that provides random values. It uses C++'s `std::default_random_engine` internally.

```cpp
#include <simulant/utils/random.h>

// Get the default instance (auto-seeded with time(NULL) on first call)
auto& rng = RandomGenerator::instance();

// Or create your own with a specific seed (useful for reproducible tests)
RandomGenerator seeded_rng(42);
```

### Generating Values

```cpp
// Integers in a range (inclusive on both ends)
int32_t i = rng.int_in_range(0, 10);    // 0 to 10 inclusive

// Floats in a range
float f = rng.float_in_range(0.0f, 1.0f);

// Any integer (full range)
int32_t any = rng.any_int();
```

### Random Points and Directions

```cpp
// Points (inside the shape)
Vec2 point2d = rng.point_in_circle(10.0f);   // diameter
Vec3 point3d = rng.point_in_sphere(10.0f);

// Points on the surface (on the boundary only)
Vec2 on_circle = rng.point_on_circle(10.0f);
Vec3 on_sphere = rng.point_on_sphere(10.0f);

// Normalized direction vectors
Vec2 dir2d = rng.direction_2d();
Vec3 dir3d = rng.direction_3d();
```

### Choosing and Shuffling

```cpp
// Pick a random element from an array
int arr[] = {10, 20, 30, 40};
int chosen = rng.choice(arr, 4);

// Pick from a vector
std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
std::string name = rng.choice(names);

// Shuffle an array in-place
int cards[] = {1, 2, 3, 4, 5};
rng.shuffle(cards, 5);

// Shuffle a vector in-place
rng.shuffle(names);

// Get a shuffled copy (original is unchanged)
std::vector<int> shuffled = rng.shuffled(original);
```

---

## 4. JSON Parsing

Source: `simulant/utils/json.h`

Simulant includes a custom JSON parser designed for memory-constrained platforms. It works directly with input streams to minimize memory usage.

### Loading and Parsing

Three entry points are available:

```cpp
#include <simulant/utils/json.h>

// From a file path (uses Simulant's VFS)
JSONIterator json = json_load(Path("config.json"));

// From a string
JSONIterator json = json_parse(R"({"key": "value"})");

// From a stream (most memory-efficient, recommended)
auto stream = vfs->open_file(Path("config.json"));
JSONIterator json = json_read(stream);
```

### Navigating Documents

Use the `[]` operator to traverse the structure. Each access returns another `JSONIterator`.

```cpp
auto json = json_parse(R"({
    "player": {
        "name": "Hero",
        "level": 5,
        "hp": 100.0,
        "alive": true,
        "inventory": ["sword", "shield", "potion"]
    }
})");

// Navigate into nested objects
auto player = json["player"];
auto name_node = player["name"];
auto level_node = player["level"];

// Extract values
optional<std::string> name = name_node->to_str();
optional<int64_t> level = level_node->to_int();
optional<float> hp = player["hp"]->to_float();
optional<bool> alive = player["alive"]->to_bool();

// Check types
player["name"]->is_str();     // true
player["level"]->is_number(); // true
player["alive"]->is_bool();   // true
player["name"]->is_array();   // false
player["name"]->is_object();  // false
player["name"]->is_null();    // false
```

### Working with Arrays

```cpp
auto inventory = player["inventory"];

// Get array size
inventory->size();  // 3

// Check it's an array
inventory->is_array();  // true

// Iterate with range-for
for (auto& item : player["inventory"]) {
    S_INFO("Item: {0}", item.to_str().value());
}

// Access by index
auto first_item = inventory[0];
first_item->to_str();  // "sword"

// Get nested iterators
auto it = inventory[0].to_iterator();
```

### Type Conversion and Defaults

```cpp
auto node = json["some_key"];

// Safe extraction with optional
if (auto val = node->to_int()) {
    int64_t i = val.value();
}

// Auto-cast helper (simpler syntax)
auto maybe_int = json_auto_cast<int>(node);
auto maybe_float = json_auto_cast<float>(node);
auto maybe_str = json_auto_cast<std::string>(node);
auto maybe_bool = json_auto_cast<bool>(node);

// Check for null
node->is_null();  // true means the value is JSON null

// Get all keys in an object
std::vector<std::string> keys = player->keys();
player->has_key("name");  // true

// Get string representation
node->repr();  // human-readable, e.g. "sword" or "100"
```

### Important Note on Memory

Each `JSONIterator` retains a reference-count on the underlying stream. Do not hold onto `JSONIterator` objects longer than necessary, especially when using `json_read()` with large files.

---

## 5. Logging

Source: `simulant/logging.h`

Simulant provides a named logging system similar to Python's `logging` module.

### Getting a Logger

```cpp
#include <simulant/logging.h>

// Get a named logger
Logger* logger = get_logger("my_module");

// Set log level
logger->set_level(LOG_LEVEL_DEBUG);   // show everything
logger->set_level(LOG_LEVEL_INFO);    // info and above
logger->set_level(LOG_LEVEL_WARN);    // warnings and errors only
logger->set_level(LOG_LEVEL_ERROR);   // errors only
logger->set_level(LOG_LEVEL_NONE);    // silence
```

### Log Levels

```cpp
LOG_LEVEL_VERBOSE   // 5 - most detailed
LOG_LEVEL_DEBUG     // 4 - debug info
LOG_LEVEL_INFO      // 3 - general information
LOG_LEVEL_WARN      // 2 - warnings
LOG_LEVEL_ERROR     // 1 - errors only
LOG_LEVEL_NONE      // 0 - no output
```

### Logging Macros

The macros automatically include the source file and line number, and support format strings using `{0}`, `{1}`, etc. placeholders.

```cpp
#include <simulant/logging.h>

S_DEBUG("Position: x={0}, y={1}", pos.x, pos.y);
S_INFO("Entity {0} spawned at {1:.2f}", entity_id, x_coord);
S_WARN("Texture not found: {0}", path);
S_ERROR("Failed to load shader: {0}", error_message);
S_VERBOSE("Frame time: {0:.3f}ms", frame_time);

// Log each message only once (useful for rare errors)
S_ERROR_ONCE("Something went wrong in {0}", function_name);
S_WARN_ONCE("Deprecated function called: {0}", func);
```

### Format Strings

Use `{N}` for positional parameters. Use `{N:.P}` to set floating-point precision to `P` decimal places.

```cpp
_F("Hello {0}, you have {1} coins").format("Player", 42);
_F("Value: {0:.3f}").format(3.14159f);  // "Value: 3.142"
```

### Output Handlers

By default, logs go to stdout/stderr. You can add file handlers:

```cpp
auto file_handler = std::make_shared<FileHandler>("mylog.txt");
logger->add_handler(file_handler);
```

### Scoped Debug Logging

For tracing function entry/exit:

```cpp
void my_function() {
    SMLT_DEBUG_SCOPED("my_function");  // logs "Enter" and "Exit"
    // ... do work ...
}  // logs "Exit: my_function" automatically
```

---

## 6. Debug Drawing

Source: `simulant/nodes/debug.h`

Every `Stage` has a `debug` property of type `Debug*` that lets you render temporary lines, rays, and points directly in the scene.

### Basic Usage

```cpp
// Draw a red line from origin to (1, 1, 1)
stage->debug->draw_line(
    Vec3(0, 0, 0),           // start
    Vec3(1, 1, 1),           // end
    Color::red(),            // color
    Seconds(),               // duration (0 = one frame)
    false                    // depth test
);

// Draw a ray (start + direction)
stage->debug->draw_ray(
    Vec3(0, 0, 0),           // origin
    Vec3(0, 1, 0),           // direction
    Color::white(),          // default color
    Seconds(),               // default: one frame
    false                    // no depth test
);

// Draw a blue point
stage->debug->draw_point(
    Vec3(5, 3, 2),           // position
    Color::blue(),           // color
    Seconds(1.0f),           // lasts 1 second
    false                    // visible through geometry
);
```

### Duration

The `duration` parameter controls how long the element persists:

- `Seconds()` (default) -- the element is drawn for one frame only. Call every frame to keep it visible.
- `Seconds(1.0f)` -- the element persists for 1 second automatically.
- `Seconds(5.0f)` -- persists for 5 seconds.

This is useful for drawing one-shot debug visuals (e.g., collision impact points) without managing their lifecycle manually.

### Configuration

```cpp
// Adjust visual size
stage->debug->set_point_size(0.05f);
stage->debug->set_line_width(0.02f);

float ps = stage->debug->point_size();
float lw = stage->debug->line_width();
```

### Practical Examples

```cpp
// Visualize a velocity vector
stage->debug->draw_ray(
    entity->translation(),
    entity->velocity().normalized() * 2.0f,
    Color::green()
);

// Draw bounding box edges
AABB aabb = mesh->aabb();
auto corners = aabb.corners();
// ... draw lines between corners ...

// Mark a collision point that persists for half a second
stage->debug->draw_point(collision_point, Color::red(), Seconds(0.5f));
```

---

## 7. Profiling

Source: `simulant/tools/profiler.h`

Simulant includes a simple scope-based profiler.

### Enabling Profiling

Profiling can be enabled in several ways:

1. **Environment variable:** Set `SIMULANT_PROFILE=1` before running.
2. **Compile-time flag:** Build with `-DSIMULANT_PROFILE` (passed to CMake).
3. **Programmatically:** Set `AppConfig::development::force_profiling = true`.

When profiling is enabled, the engine may also disable vsync and frame limiting to get accurate measurements.

### Using the Profiler

```cpp
void my_function() {
    S_PROFILE_SECTION("MyFunction");
    // ... code to measure ...

    S_PROFILE_SECTION("SubWork");
    // ... sub-section code ...

    S_PROFILE_SUBSECTION("Nested");
    // ... nested work ...
}
```

`S_PROFILE_SUBSECTION` creates a hierarchical entry under the current section.

### Dumping Results

```cpp
S_PROFILE_START_FRAME();   // Reset counters at start of frame
// ... measured code ...
S_PROFILE_DUMP_TO_STDOUT(); // Print results to console
```

Example output:
```
MyFunction                        XXus
SubWork                           XXus
SubWork / Nested                  XXus
```

### Conditional Profiling

The `_S_PROFILE_SECTION` and `_S_PROFILE_SUBSECTION` macros are only active when `SIMULANT_PROFILE` is defined. Use these for production-safe profiling:

```cpp
void expensive_operation() {
    _S_PROFILE_SECTION("ExpensiveOp");
    // ... this section is compiled out unless profiling is enabled ...
}
```

### Dreamcast Profiler

On Dreamcast, the built-in sampling profiler runs in a background thread. When running via `dcload` with profiling enabled, it generates a `/pc/gmon.out` file that can be analyzed with `gprof`:

```bash
sh-elf-gprof -b -p samples/mygame.elf.debug gmon.out
```

Use the `.debug` ELF file (not the stripped version) for readable function names.

---

## 8. Command Line Arguments

Source: `simulant/arg_parser.h`

Simulant provides an argument parser for defining and reading command-line options.

### Defining Arguments

Define arguments in your `Application` subclass constructor:

```cpp
// In your Application constructor
args->define_arg("--fullscreen", ARG_TYPE_BOOLEAN, "Run in fullscreen mode");
args->define_arg("--resolution", ARG_TYPE_STRING, "Window resolution", "res", ARG_COUNT_ONE);
args->define_arg("--threads", ARG_TYPE_INTEGER, "Number of worker threads", "threads", ARG_COUNT_ZERO_OR_ONE);
args->define_arg("--log-file", ARG_TYPE_STRING, "Log output file", "file", ARG_COUNT_ONE);
```

Parameters to `define_arg`:
1. **Name** -- the flag (must start with `--`)
2. **Type** -- `ARG_TYPE_BOOLEAN`, `ARG_TYPE_STRING`, `ARG_TYPE_INTEGER`, or `ARG_TYPE_FLOAT`
3. **Help text** -- shown with `--help`
4. **Variable name** (optional) -- the short name used with `arg_value()`
5. **Count** (optional) -- how many values the argument accepts:
   - `ARG_COUNT_ONE` (default)
   - `ARG_COUNT_ZERO_OR_ONE`
   - `ARG_COUNT_ONE_OR_MANY`
   - `ARG_COUNT_ZERO_OR_MANY`

### Accessing Values

```cpp
// With optional return (returns optional<T>)
auto fullscreen = args->arg_value<bool>("fullscreen");
if (fullscreen.has_value() && fullscreen.value()) {
    // user passed --fullscreen
}

// With a default value
auto threads = args->arg_value<int>("threads", 4);  // defaults to 4

// For arguments that accept multiple values
auto files = args->arg_value_list<std::string>("input-files");
for (const auto& f : files) {
    S_INFO("Processing: {0}", f);
}
```

### Passing Arguments

Make sure to pass `argc` and `argv` to `Application::run()`:

```cpp
int main(int argc, char* argv[]) {
    MyGame game;
    return game.run(argc, argv);
}
```

### Help Output

Running with `--help` automatically prints all defined arguments and their descriptions.

---

## 9. Time and Timing

Source: `simulant/time_keeper.h`

`TimeKeeper` manages frame timing, delta time, and fixed-step updates.

### Accessing Time Data

The `TimeKeeper` is available on the application object:

```cpp
TimeKeeper* tk = app->time_keeper;

// Delta time (time since last frame, in seconds)
float dt = tk->delta_time();

// Unscaled delta (not affected by time_scale)
float unscaled_dt = tk->unscaled_delta_time();

// Total elapsed time since the TimeKeeper started
float elapsed = tk->total_elapsed_seconds();

// Fixed step interval (for physics)
float fixed_step = tk->fixed_step();

// Remaining time until the next fixed step
float remainder = tk->fixed_step_remainder();
```

### Time Scale (Slow Motion / Pause)

```cpp
// Slow motion (half speed)
tk->set_time_scale(0.5f);

// Fast forward
tk->set_time_scale(2.0f);

// Pause (delta_time becomes 0, but update() still fires)
tk->set_time_scale(0.0f);

// Normal speed
tk->set_time_scale(1.0f);
```

### Restarting the Timer

```cpp
tk->restart();  // Reset all counters to zero
```

### High-Resolution Timestamp

```cpp
#include <simulant/time_keeper.h>

uint64_t now = TimeKeeper::now_in_us();  // microseconds since epoch
```

### Fixed-Step Mode

```cpp
if (tk->use_fixed_step()) {
    // Physics step will be called at a fixed interval
    // based on the configured fixed_step value
}
```

---

## 10. Simplex Noise

Source: `simulant/utils/simplex.h`

The `Simplex` class generates gradient noise in 2D, 3D, and 4D.

```cpp
#include <simulant/utils/simplex.h>

// Auto-seeded with current time
auto simplex = Simplex::create();

// Or seed explicitly
auto simplex = Simplex::create(42);

// 2D noise (returns values roughly in [-1, 1])
float n2d = simplex->noise(x, y);

// 3D noise
float n3d = simplex->noise(x, y, z);

// 4D noise
float n4d = simplex->noise(x, y, z, w);
```

### Typical Usage: Terrain Heightmap

```cpp
auto noise = Simplex::create(12345);

for (int x = 0; x < width; x++) {
    for (int z = 0; z < height; z++) {
        float scale = 0.01f;
        float h = noise->noise(x * scale, z * scale);
        // h is roughly in [-1, 1], map to your height range
        float height = (h + 1.0f) * 0.5f * max_height;
        terrain[x][z] = height;
    }
}
```

---

## 11. Base64 Encoding

Source: `simulant/utils/base64.h`

Simulant provides Base64 **decoding**. There is no built-in encoder.

```cpp
#include <simulant/utils/base64.h>

std::string encoded = "SGVsbG8gV29ybGQh";
auto decoded = base64_decode(encoded);

if (decoded.has_value()) {
    S_INFO("Decoded: {0}", decoded.value());  // "Hello World!"
} else {
    S_ERROR("Invalid base64 input");
}
```

If the input contains invalid characters, `base64_decode` returns `no_value`.

---

## 12. String and Vector Utilities

### String Utilities

Source: `simulant/utils/string.h`

```cpp
#include <simulant/utils/string.h>

// Prefix/suffix checks
starts_with("hello world", "hello");  // true
ends_with("hello world", "world");    // true
contains("hello world", "lo wo");     // true
count("banana", "na");                // 2

// Split
auto parts = split("one,two,three", ",");    // {"one", "two", "three"}
auto first_two = split("a:b:c:d", ":", 2);   // {"a", "b", "c:d"}

// Strip whitespace
strip("  hello  \t\n");  // "hello"
strip("---hello---", "-");  // "hello"

// Replace
replace_all("foo bar foo", "foo", "baz");  // "baz bar baz"

// Case conversion
lower_case("Hello WORLD");  // "hello world"
upper_case("Hello WORLD");  // "world"
```

### Formatter

Source: `simulant/utils/formatter.h`

```cpp
#include <simulant/utils/formatter.h>

// Using the _F shortcut
std::string msg = _F("Player {0} has {1} health").format(name, hp);
std::string precise = _F("Speed: {0:.2f} m/s").format(velocity);

// Available as _F globally
S_DEBUG(_F("Loaded {0} assets from {1}").format(count, dir));
```

### LimitedString

Source: `simulant/utils/limited_string.h`

A fixed-capacity string that avoids heap allocation. Useful for performance-critical code or embedded systems.

```cpp
#include <simulant/utils/limited_string.h>

LimitedString<32> name("player_01");  // max 32 chars
name.str();        // std::string
name.c_str();      // const char*
name.length();     // current length
name.capacity();   // 32
name.empty();      // false
name.clear();
```

### LimitedVector

Source: `simulant/utils/limited_vector.h`

A size-limited vector with static allocation (no heap).

```cpp
#include <simulant/utils/limited_vector.h>

LimitedVector<int, 16> vec;
vec.push_back(10);
vec.push_back(20);
vec.size();      // 2
vec.capacity();  // 16
vec.empty();     // false

for (auto& v : vec) {
    // iterate
}

vec.pop_back();
vec.clear();

// push_back returns false if full
bool ok = vec.push_back(30);
```

### Lerp Functions

Source: `simulant/math/lerp.h`

```cpp
#include <simulant/math/lerp.h>

// Scalar
float val = lerp(0.0f, 100.0f, 0.5f);  // 50.0

// Vectors
Vec3 result = lerp(start, end, 0.5f);
Vec2 result2d = lerp(a2d, b2d, t);

// Quaternions
Quaternion q = lerp(q1, q2, t);

// Degrees / Radians
Degrees d = lerp(d1, d2, t);
Radians r = lerp(r1, r2, t);

// Smooth lerp (frame-rate independent)
// p = target precision (e.g. 0.01 = 1% remaining), t = expected time to reach target
Vec3 smooth = lerp_smooth(from, to, dt, 0.01f, 1.0f);
```

### Float10 Encoding

Source: `simulant/utils/float.h`

For compact float storage (10-bit fixed-point, max value 64512).

```cpp
#include <simulant/utils/float.h>

float val = 123.5f;
auto f10 = float10_from_float(val);
if (f10.has_value()) {
    float decoded = float10_to_float(f10.value());
}
```

---

## Quick Reference: Header Files

| Utility | Header |
|---------|--------|
| Vec2, Vec3, Vec4 | `<simulant/math/vec2.h>`, `<simulant/math/vec3.h>`, `<simulant/math/vec4.h>` |
| Quaternion | `<simulant/math/quaternion.h>` |
| Mat3, Mat4 | `<simulant/math/mat3.h>`, `<simulant/math/mat4.h>` |
| Ray | `<simulant/math/ray.h>` |
| AABB | `<simulant/math/aabb.h>` |
| Plane | `<simulant/math/plane.h>` |
| Degrees, Radians, Euler | `<simulant/math/degrees.h>`, `<simulant/math/radians.h>`, `<simulant/math/euler.h>` |
| Math utils (PI, clamp, lerp, etc.) | `<simulant/math/utils.h>`, `<simulant/math/lerp.h>` |
| Random numbers | `<simulant/utils/random.h>` |
| JSON parsing | `<simulant/utils/json.h>` |
| Logging | `<simulant/logging.h>` |
| Debug drawing | Available via `stage->debug` (no include needed, part of `simulant.h`) |
| Profiler | `<simulant/tools/profiler.h>` |
| Arg parsing | `<simulant/arg_parser.h>` |
| TimeKeeper | `<simulant/time_keeper.h>` |
| Simplex noise | `<simulant/utils/simplex.h>` |
| Base64 | `<simulant/utils/base64.h>` |
| String utilities | `<simulant/utils/string.h>` |
| Formatter | `<simulant/utils/formatter.h>` |
| LimitedString | `<simulant/utils/limited_string.h>` |
| LimitedVector | `<simulant/utils/limited_vector.h>` |
| Float10 | `<simulant/utils/float.h>` |
