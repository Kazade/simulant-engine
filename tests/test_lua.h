#pragma once

namespace {
using namespace smlt;

const char* test_node = R"(
-- smlt.define_node() handles all the boilerplate: class table setup,
-- Meta registration, and the :new(scene) wrapper constructor.
BallNode = smlt.define_node("ball_node")

function BallNode:on_update(dt)
    print("Update from Lua")
end
)";

// Lua snippet used by test_lua_transform to exercise the math/transform
// bindings.  It defines a node whose on_create callback:
//   1. Creates Vec3 / Quaternion / Degrees values.
//   2. Reads and writes transform properties (position, rotation, etc.).
//   3. Stores results in a global table so the C++ test can inspect them.
const char* test_transform_node = R"(
TransformNode = smlt.define_node("transform_node")

function TransformNode:on_create()
    local t = self.transform

    -- Vec3 construction and property access
    local v = smlt.Vec3(1, 2, 3)
    assert(v.x == 1 and v.y == 2 and v.z == 3, "Vec3 construction failed")

    -- Vec3 zero / static helpers
    local z = smlt.Vec3()
    assert(z.x == 0 and z.y == 0 and z.z == 0, "Vec3 default ctor failed")

    -- Vec3 single-float fill
    local ones = smlt.Vec3(1)
    assert(ones.x == 1 and ones.y == 1 and ones.z == 1, "Vec3(f) ctor failed")

    -- Vec3 methods
    local len = smlt.Vec3(3, 4, 0):length()
    assert(math.abs(len - 5) < 0.001, "Vec3:length() failed")

    -- Degrees construction
    local d = smlt.Degrees(90)
    assert(math.abs(d:to_float() - 90) < 0.001, "Degrees construction failed")

    -- Quaternion construction (identity)
    local q_id = smlt.Quaternion()
    assert(math.abs(q_id.w - 1) < 0.001, "Quaternion identity ctor failed")

    -- Quaternion construction (xyzw)
    local q = smlt.Quaternion(0, 0, 0, 1)
    assert(math.abs(q.w - 1) < 0.001, "Quaternion(xyzw) ctor failed")

    -- Transform: set and read back world-space position
    t.position = smlt.Vec3(10, 20, 30)
    local p = t.position
    assert(math.abs(p.x - 10) < 0.01 and
           math.abs(p.y - 20) < 0.01 and
           math.abs(p.z - 30) < 0.01,
           "Transform.position round-trip failed")

    -- Transform: set and read back local translation
    t.translation = smlt.Vec3(5, 6, 7)
    local tr = t.translation
    assert(math.abs(tr.x - 5) < 0.01, "Transform.translation round-trip failed")

    -- Transform: direction helpers exist and return Vec3-like objects
    local fwd = t:forward()
    assert(fwd ~= nil, "Transform:forward() returned nil")

    -- Store a sentinel so the C++ side can confirm on_create ran
    TransformNodeResult = { ok = true }
    return true
end
)";

class LuaTests: public test::SimulantTestCase {
public:
    void test_registering_lua_stage_node() {
        assert_true(scene->register_stage_node(test_node, "BallNode"));
        auto maybe_info = scene->registered_stage_node_info("ball_node");
        assert_true(maybe_info);
        auto info = maybe_info.value();
        assert_equal(info.name, std::string("ball_node"));
    }

    void test_spawning_lua_stage_node() {
        test_registering_lua_stage_node();

        auto ball = scene->create_child("ball_node");
        assert_is_not_null(ball);
    }

    void test_lua_transform() {
        // Register and spawn a node whose on_create exercises the
        // Vec3 / Quaternion / Degrees / Transform Lua bindings.
        //
        // The Lua script uses assert() inside on_create; if any assertion
        // fails it raises a Lua error, on_create returns false, and
        // create_child returns nullptr.  A non-null result therefore proves
        // every Lua assertion passed.
        assert_true(scene->register_stage_node(test_transform_node,
                                               "TransformNode"));

        auto node = scene->create_child("transform_node");
        assert_is_not_null(node);
    }
};

} // namespace
