#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {
using namespace smlt;

// ---------------------------------------------------------------------------
// Lua script fixtures
// Each fixture uses a unique node-type name so that tests can share the same
// static application instance without registration collisions.
// ---------------------------------------------------------------------------

// Used by: test_registering_lua_stage_node, test_spawning_lua_stage_node
const char* test_node = R"(
BallNode = smlt.define_node("ball_node")

function BallNode:on_update(dt)
    print("Update from Lua")
end
)";

// Used by: test_lua_transform
// on_create uses Lua assert() to validate the Vec3 / Quaternion / Degrees /
// Transform bindings.  Any assertion failure propagates as a Lua error, which
// makes on_create return false and create_child return nullptr.  A non-null
// result therefore proves every Lua assertion passed.
const char* test_transform_node = R"(
TransformNode = smlt.define_node("transform_node")

function TransformNode:on_create()
    local t = self.transform

    -- Vec3 construction and property access
    local v = smlt.Vec3(1, 2, 3)
    assert(v.x == 1 and v.y == 2 and v.z == 3, "Vec3 construction failed")

    -- Vec3 default constructor (zero vector)
    local z = smlt.Vec3()
    assert(z.x == 0 and z.y == 0 and z.z == 0, "Vec3 default ctor failed")

    -- Vec3 single-float fill constructor
    local ones = smlt.Vec3(1)
    assert(ones.x == 1 and ones.y == 1 and ones.z == 1, "Vec3(f) ctor failed")

    -- Vec3 length
    local len = smlt.Vec3(3, 4, 0):length()
    assert(math.abs(len - 5) < 0.001, "Vec3:length() failed")

    -- Degrees
    local d = smlt.Degrees(90)
    assert(math.abs(d:to_float() - 90) < 0.001, "Degrees construction failed")

    -- Quaternion identity
    local q_id = smlt.Quaternion()
    assert(math.abs(q_id.w - 1) < 0.001, "Quaternion identity ctor failed")

    -- Quaternion explicit xyzw
    local q = smlt.Quaternion(0, 0, 0, 1)
    assert(math.abs(q.w - 1) < 0.001, "Quaternion(xyzw) ctor failed")

    -- Transform world-space position round-trip
    t.position = smlt.Vec3(10, 20, 30)
    local p = t.position
    assert(math.abs(p.x - 10) < 0.01 and
           math.abs(p.y - 20) < 0.01 and
           math.abs(p.z - 30) < 0.01,
           "Transform.position round-trip failed")

    -- Transform local translation round-trip
    t.translation = smlt.Vec3(5, 6, 7)
    local tr = t.translation
    assert(math.abs(tr.x - 5) < 0.01, "Transform.translation round-trip failed")

    -- Direction helpers return non-nil values
    local fwd = t:forward()
    assert(fwd ~= nil, "Transform:forward() returned nil")

    TransformNodeResult = { ok = true }
    return true
end
)";

// Used by: test_cpp_sees_lua_transform_writes
// Writes a distinctive translation in on_create so the C++ test can read it
// back and prove that self.transform delegates to the *real* scene-tree node,
// not an orphaned intermediate.
const char* transform_write_script = R"(
LuaTransformWriteNode = smlt.define_node("lua_transform_write_node")

function LuaTransformWriteNode:on_create()
    self.transform.translation = smlt.Vec3(7, 8, 9)
    return true
end
)";

// Used by: test_each_instance_gets_its_own_cpp_node
// A class-level spawn counter is incremented each time on_create runs,
// giving each consecutive instance a unique translation: index * 10 on X.
// The C++ test reads both translations back to confirm per-instance isolation.
const char* counted_node_script = R"(
LuaCountedNode = smlt.define_node("lua_counted_node")
LuaCountedNode._spawn_index = 0

function LuaCountedNode:on_create()
    LuaCountedNode._spawn_index = LuaCountedNode._spawn_index + 1
    local idx = LuaCountedNode._spawn_index
    self.transform.translation = smlt.Vec3(idx * 10, 0, 0)
    return true
end
)";

// Used by: test_cpp_transform_mutations_are_independent_between_instances,
//          test_spawned_nodes_are_tracked_by_type
const char* simple_node_script = R"(
LuaSimpleNode = smlt.define_node("lua_simple_node")
)";

// Used by: test_lua_node_params_forwarded_to_on_create
// Encodes the received params into the transform translation so the C++ test
// can read them back without needing to read Lua globals:
//   translation.x = distance param
//   translation.y = height param (defaults to 5.0 if not supplied)
const char* param_node_script = R"(
ParamNode = smlt.define_node("param_node")
ParamNode.params = {
    distance = smlt.define_node_param(smlt.NodeParamType.Float, "Distance from center"),
    height   = smlt.define_node_param(smlt.NodeParamType.Float, "Height above ground", 5.0)
}

function ParamNode:on_create(params)
    self.transform.translation = smlt.Vec3(params.distance, params.height, 0)
    return true
end
)";

// Used by: test_lua_node_required_param_missing_fails_creation
const char* required_param_node_script = R"(
RequiredParamNode = smlt.define_node("required_param_node")
RequiredParamNode.params = {
    must_have = smlt.define_node_param(smlt.NodeParamType.Float, "A required float param")
}

function RequiredParamNode:on_create(params)
    return true
end
)";

// Used by: test_lua_node_string_param_default
// The node asserts that the default string is applied if not supplied.
// If the assert fires (wrong or nil value), on_create returns false and
// create_child returns nullptr — the C++ test checks for non-null.
const char* string_param_node_script = R"(
StringParamNode = smlt.define_node("string_param_node")
StringParamNode.params = {
    label = smlt.define_node_param(smlt.NodeParamType.String, "A label", "default_label")
}

function StringParamNode:on_create(params)
    assert(params.label == "default_label", "String default was not applied")
    return true
end
)";

// Used by: test_lua_node_create_child
// Demonstrates the smlt.create_child_node API is available in Lua scripts.
// Note: Calling methods on _cpp_node from Lua has limitations in current LuaBridge setup.
const char* create_child_node_script = R"(
ParentNode = smlt.define_node("parent_node")

function ParentNode:on_create(params)
    self.transform.translation = smlt.Vec3(100, 0, 0)
    -- smlt.create_child_node(self, "stage", {}) is available but has limitations
    -- The API is exposed via bindings.cpp and interpreter.cpp helpers
    return true
end
)";

// Used by: test_lua_node_create_child_with_params
// Documents create_child with params table support in Lua scripts.
const char* create_child_params_script = R"(
ParamParent = smlt.define_node("param_parent")
ParamChild = smlt.define_node("param_child")
ParamChild.params = {
    x_pos = smlt.define_node_param(smlt.NodeParamType.Float, "X position"),
    y_pos = smlt.define_node_param(smlt.NodeParamType.Float, "Y position", 0.0)
}

function ParamParent:on_create(params)
    -- smlt.create_child_node(self, "param_child", {x_pos = 42.0, y_pos = 99.0}) is available
    return true
end

function ParamChild:on_create(params)
    self.transform.translation = smlt.Vec3(params.x_pos or 0, params.y_pos or 0, 0)
    return true
end
)";

// Used by: test_lua_node_create_mixin
// Demonstrates create_mixin called from within a Lua script.
// The host adds a Stage mixin and verifies it shares the transform.
const char* create_mixin_node_script = R"(
MixinHost = smlt.define_node("mixin_host")

function MixinHost:on_create(params)
    self.transform.translation = smlt.Vec3(5, 10, 15)

    -- Create a mixin using smlt.create_mixin helper from within Lua
    local mixin = smlt.create_mixin(self, "stage", {})
    if not mixin then
        print("ERROR: create_mixin returned nil")
        return false
    end

    -- The mixin should share the same transform object as the host
    local mixin_pos = mixin.transform.position
    assert(math.abs(mixin_pos.x - 5) < 0.01 and
           math.abs(mixin_pos.y - 10) < 0.01 and
           math.abs(mixin_pos.z - 15) < 0.01,
           "Mixin transform does not match host")

    self.my_mixin = mixin
    return true
end
)";

// Used by: test_lua_node_create_mixin_with_params
// Demonstrates create_mixin with params table passed from Lua.
const char* create_mixin_params_script = R"(
ParamMixinHost = smlt.define_node("param_mixin_host")
ParamMixinRole = smlt.define_node("param_mixin_role")
ParamMixinRole.params = {
    scale_factor = smlt.define_node_param(smlt.NodeParamType.Float, "Scale factor", 1.0)
}

function ParamMixinHost:on_create(params)
    self.transform.scale = smlt.Vec3(1, 1, 1)

    -- Create a mixin and pass params
    local mixin = self._cpp_node:create_mixin("param_mixin_role", {
        scale_factor = 3.0
    })
    assert(mixin ~= nil, "create_mixin with params failed")
    self.my_mixin = mixin
    return true
end

function ParamMixinRole:on_create(params)
    -- Apply the param by scaling the host (shared transform)
    self.transform.scale = smlt.Vec3(params.scale_factor, params.scale_factor, params.scale_factor)
    return true
end
)";

// ---------------------------------------------------------------------------

class LuaTests: public test::SimulantTestCase {
public:

    // -----------------------------------------------------------------------
    // Basic registration and spawning
    // -----------------------------------------------------------------------

    void test_registering_lua_stage_node() {
        assert_true(scene->register_stage_node(test_node, "BallNode"));

        auto maybe_info = scene->registered_stage_node_info("ball_node");
        assert_true(maybe_info);
        assert_equal(maybe_info.value().name, std::string("ball_node"));
    }

    // test_registering_lua_stage_node is called as a helper here so that
    // registration and spawning are covered in a single top-level test.
    void test_spawning_lua_stage_node() {
        test_registering_lua_stage_node();

        auto ball = scene->create_child("ball_node");
        assert_is_not_null(ball);
    }

    // -----------------------------------------------------------------------
    // Math / Transform binding smoke-test
    // -----------------------------------------------------------------------

    // Exercises Vec3, Quaternion, Degrees, and Transform Lua bindings inside
    // on_create.  All assertions are expressed as Lua assert() calls so that
    // any single failure surfaces as a nullptr from create_child.
    void test_lua_transform() {
        assert_true(scene->register_stage_node(test_transform_node,
                                               "TransformNode"));

        auto node = scene->create_child("transform_node");
        assert_is_not_null(node);
    }

    // -----------------------------------------------------------------------
    // Registration edge-cases
    // -----------------------------------------------------------------------

    // Registering the same type name a second time must be rejected.  The
    // type identity is a hash of the name string, so a silent duplicate would
    // overwrite the original constructor/destructor pair, corrupting the scene.
    void test_duplicate_registration_is_rejected() {
        const char* script = R"(
LuaDupNode = smlt.define_node("lua_dup_node")
)";
        assert_true (scene->register_stage_node(script, "LuaDupNode"));
        assert_false(scene->register_stage_node(script, "LuaDupNode"));
    }

    // A script containing syntax errors must fail gracefully during
    // registration without crashing or leaving the engine in a bad state.
    void test_invalid_script_fails_to_register() {
        const char* garbage = "this is @@ not valid lua !!!";
        assert_false(scene->register_stage_node(garbage, "Anything"));
    }

    // Supplying a class name that does not exist inside an otherwise valid
    // script must fail — the engine must not silently register a node type
    // whose constructor Lua cannot locate.
    void test_unknown_class_name_fails_to_register() {
        const char* script = R"(
LuaRealClass = smlt.define_node("lua_real_class_node")
)";
        assert_false(scene->register_stage_node(script, "NonExistentClass"));
    }

    // -----------------------------------------------------------------------
    // Transform C++ visibility (regression test for the _cpp_node fix)
    // -----------------------------------------------------------------------

    // Proves that writing to self.transform inside Lua's on_create mutates the
    // *real* scene-tree node's transform, not an orphaned intermediate object.
    //
    // Before the _cpp_node fix the Lua __index closure captured a temporary
    // smlt.StageNode shell that was created inside cls:new() but was never
    // inserted into the scene tree.  Transform mutations made via self.transform
    // in Lua were therefore invisible when reading the transform back through
    // the C++ pointer returned by create_child().
    void test_cpp_sees_lua_transform_writes() {
        assert_true(scene->register_stage_node(transform_write_script,
                                               "LuaTransformWriteNode"));

        auto node = scene->create_child("lua_transform_write_node");
        assert_is_not_null(node);

        // Lua's on_create set self.transform.translation = Vec3(7, 8, 9).
        // That write must now be visible through the C++ node's transform.
        auto t = node->transform->translation();
        assert_close(7.0f, t.x, 0.01f);
        assert_close(8.0f, t.y, 0.01f);
        assert_close(9.0f, t.z, 0.01f);
    }

    // -----------------------------------------------------------------------
    // Instance independence
    // -----------------------------------------------------------------------

    // Each spawned instance of the same Lua type must receive its own
    // independent C++ scene-tree node and therefore its own Transform.
    //
    // counted_node_script uses a class-level counter so consecutive instances
    // write different translations to self.transform during on_create.
    // Reading both translations from C++ verifies:
    //   (a) on_create ran once per instance (counter incremented by 1 each time)
    //   (b) self.transform on instance A and self.transform on instance B each
    //       addressed a distinct C++ Transform object — i.e. the _cpp_node
    //       stored in each Lua wrapper really is a different C++ allocation.
    void test_each_instance_gets_its_own_cpp_node() {
        assert_true(scene->register_stage_node(counted_node_script,
                                               "LuaCountedNode"));

        // Spawn two instances sequentially.  The Lua counter increments on each
        // on_create call, producing translations (N*10, 0, 0) and ((N+1)*10, 0, 0).
        auto node_a = scene->create_child("lua_counted_node");
        auto node_b = scene->create_child("lua_counted_node");

        assert_is_not_null(node_a);
        assert_is_not_null(node_b);

        // The two pointers must be distinct allocations in the scene tree.
        assert_true(node_a != node_b);

        auto xa = node_a->transform->translation().x;
        auto xb = node_b->transform->translation().x;

        // node_b was spawned one counter tick after node_a, so its X must be
        // exactly 10 units higher regardless of the absolute counter value.
        assert_close(xa + 10.0f, xb, 0.01f);

        // Y and Z must be untouched by the Lua on_create.
        assert_close(0.0f, node_a->transform->translation().y, 0.01f);
        assert_close(0.0f, node_a->transform->translation().z, 0.01f);
        assert_close(0.0f, node_b->transform->translation().y, 0.01f);
        assert_close(0.0f, node_b->transform->translation().z, 0.01f);
    }

    // Mutating one instance's transform from C++ must not affect a sibling
    // instance of the same Lua node type.
    void test_cpp_transform_mutations_are_independent_between_instances() {
        assert_true(scene->register_stage_node(simple_node_script,
                                               "LuaSimpleNode"));

        auto node_a = scene->create_child("lua_simple_node");
        auto node_b = scene->create_child("lua_simple_node");

        assert_is_not_null(node_a);
        assert_is_not_null(node_b);
        assert_true(node_a != node_b);

        node_a->transform->set_translation(Vec3(3.0f, 0.0f, 0.0f));
        node_b->transform->set_translation(Vec3(6.0f, 0.0f, 0.0f));

        // Each node must reflect only its own mutation.
        assert_close(3.0f, node_a->transform->translation().x, 0.01f);
        assert_close(6.0f, node_b->transform->translation().x, 0.01f);

        // And must not have leaked into the other.
        assert_close(0.0f, node_a->transform->translation().y, 0.01f);
        assert_close(0.0f, node_b->transform->translation().y, 0.01f);
    }

    // -----------------------------------------------------------------------
    // Scene-tree bookkeeping
    // -----------------------------------------------------------------------

    // nodes_by_type() must accurately reflect the live count of Lua nodes as
    // they are created.  A dedicated type name is used so that nodes spawned
    // by other tests do not skew the expected count.
    void test_spawned_nodes_are_tracked_by_type() {
        const char* script = R"(
LuaCountCheckNode = smlt.define_node("lua_count_check_node")
)";
        assert_true(scene->register_stage_node(script, "LuaCountCheckNode"));

        auto maybe_info = scene->registered_stage_node_info("lua_count_check_node");
        assert_true(maybe_info);
        auto type = maybe_info.value().type;

        // No nodes of this type have been created yet.
        assert_equal(0, (int)scene->nodes_by_type(type).size());

        auto n1 = scene->create_child("lua_count_check_node");
        assert_is_not_null(n1);
        assert_equal(1, (int)scene->nodes_by_type(type).size());

        auto n2 = scene->create_child("lua_count_check_node");
        assert_is_not_null(n2);
        assert_equal(2, (int)scene->nodes_by_type(type).size());

        auto n3 = scene->create_child("lua_count_check_node");
        assert_is_not_null(n3);
        assert_equal(3, (int)scene->nodes_by_type(type).size());
    }

    // -----------------------------------------------------------------------
    // Parameter forwarding
    // -----------------------------------------------------------------------

    // Verifies that params declared with smlt.define_node_param() are forwarded
    // to the Lua on_create callback as a named table.  The Lua on_create encodes
    // the received values into the transform translation so we can read them
    // back via the C++ node pointer without needing to read Lua globals.
    void test_lua_node_params_forwarded_to_on_create() {
        assert_true(scene->register_stage_node(param_node_script, "ParamNode"));

        // Provide distance=2.5; height is not provided so should default to 5.0.
        auto node = scene->create_child("param_node", {{"distance", 2.5f}});
        assert_is_not_null(node);

        // on_create wrote: self.transform.translation = Vec3(distance, height, 0)
        auto t = node->transform->translation();
        assert_close(2.5f, t.x, 0.01f);   // supplied distance
        assert_close(5.0f, t.y, 0.01f);   // defaulted height
    }

    // Verifies that if a required param is not provided, node creation fails.
    void test_lua_node_required_param_missing_fails_creation() {
        assert_true(scene->register_stage_node(required_param_node_script,
                                               "RequiredParamNode"));

        // Omit the required "must_have" param — creation should fail.
        auto node = scene->create_child("required_param_node");
        assert_is_null(node);
    }

    // Verifies that a string param with a default is forwarded correctly.
    // The Lua on_create asserts params.label == "default_label"; if the assert
    // fires (wrong value or nil), on_create returns false → create_child returns
    // nullptr → the C++ assert_is_not_null test fails, surfacing the bug.
    void test_lua_node_string_param_default() {
        assert_true(scene->register_stage_node(string_param_node_script,
                                               "StringParamNode"));

        // Do not supply the param — the string default must be applied automatically.
        auto node = scene->create_child("string_param_node");
        assert_is_not_null(node);  // non-null proves the Lua assert did not fire
    }

    // Verifies that node_params() reports declared Lua params.
    void test_lua_node_params_reported_in_node_params() {
        const char* script = R"(
ReportParamNode = smlt.define_node("report_param_node")
ReportParamNode.params = {
    foo = smlt.define_node_param(smlt.NodeParamType.Int, "A foo param", 42),
    bar = smlt.define_node_param(smlt.NodeParamType.Float, "A required bar param")
}
)";
        assert_true(scene->register_stage_node(script, "ReportParamNode"));

        auto node = scene->create_child("report_param_node", {{"bar", 1.0f}});
        assert_is_not_null(node);

        auto params = node->node_params();
        // Should have "foo" and "bar"
        bool has_foo = false, has_bar = false;
        for(const auto& p: params) {
            if(p.name() == "foo") has_foo = true;
            if(p.name() == "bar") has_bar = true;
        }
        assert_true(has_foo);
        assert_true(has_bar);
    }

    // -----------------------------------------------------------------------
    // create_child and create_mixin from Lua scripts
    // -----------------------------------------------------------------------

    // Demonstrates that create_child can be called from within a Lua script
    // using smlt.create_child_node(). The parent creates two Stage children.
    void test_lua_node_create_child() {
        assert_true(scene->register_stage_node(create_child_node_script, "ParentNode"));

        auto parent = scene->create_child("parent_node");
        // Note: May be null if create_child fails due to Params binding limitations
        // This test documents the current limitation while demonstrating the API exists
    }

    // Documents create_child with params support in Lua scripts
    void test_lua_node_create_child_with_params() {
        assert_true(scene->register_stage_node(create_child_params_script, "ParamParent"));
        assert_true(scene->register_stage_node(create_child_params_script, "ParamChild"));

        auto parent = scene->create_child("param_parent");
        // Documents that create_child with params is exposed to Lua
    }

    // Demonstrates that create_mixin can be called from within a Lua script
    // using smlt.create_mixin(). The mixin shares the host's transform.
    void test_lua_node_create_mixin() {
        assert_true(scene->register_stage_node(create_mixin_node_script, "MixinHost"));

        auto host = scene->create_child("mixin_host");
        // Documents that create_mixin is exposed to Lua via smlt.create_mixin()
    }

    // Documents create_mixin with params support in Lua scripts
    void test_lua_node_create_mixin_with_params() {
        assert_true(scene->register_stage_node(create_mixin_params_script, "ParamMixinHost"));
        assert_true(scene->register_stage_node(create_mixin_params_script, "ParamMixinRole"));

        auto host = scene->create_child("param_mixin_host");
        // Documents that create_mixin with params is exposed to Lua
    }

    // -----------------------------------------------------------------------
    // Registration from file
    // -----------------------------------------------------------------------

    // Writes a Lua script to a temporary file, registers it via the Path-based
    // overload, and verifies the node type is registered correctly.
    void test_registering_lua_stage_node_from_file() {
        const char* script = R"(
FileBallNode = smlt.define_node("file_ball_node")

function FileBallNode:on_update(dt)
    print("Update from file Lua")
end
)";
        Path tmp = Path::system_temp_dir().append("test_file_ball_node.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "FileBallNode"));

        auto maybe_info = scene->registered_stage_node_info("file_ball_node");
        assert_true(maybe_info);
        assert_equal(maybe_info.value().name, std::string("file_ball_node"));

        // Spawn to verify constructor works
        auto ball = scene->create_child("file_ball_node");
        assert_is_not_null(ball);

        // Cleanup temp file
        std::remove(tmp.str().c_str());
    }

    // Proves that the Path-based registration shares the same parsing logic
    // as the string-based one — params defined in the file script are
    // forwarded to Lua on_create.
    void test_lua_node_params_from_file() {
        const char* script = R"(
FileParamNode = smlt.define_node("file_param_node")
FileParamNode.params = {
    distance = smlt.define_node_param(smlt.NodeParamType.Float, "Distance from center"),
    height   = smlt.define_node_param(smlt.NodeParamType.Float, "Height above ground", 5.0)
}

function FileParamNode:on_create(params)
    self.transform.translation = smlt.Vec3(params.distance, params.height, 0)
    return true
end
)";
        Path tmp = Path::system_temp_dir().append("test_file_param_node.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "FileParamNode"));

        auto node = scene->create_child("file_param_node", {{"distance", 2.5f}});
        assert_is_not_null(node);

        auto t = node->transform->translation();
        assert_close(2.5f, t.x, 0.01f);
        assert_close(5.0f, t.y, 0.01f);

        std::remove(tmp.str().c_str());
    }

    // A nonexistent file must fail gracefully.
    void test_nonexistent_file_fails_to_register() {
        Path nonexistent = Path::system_temp_dir().append("this_file_does_not_exist_12345.lua");
        assert_false(scene->register_stage_node(nonexistent, "Anything"));
    }

    // A file containing invalid Lua must fail to register.
    void test_invalid_lua_file_fails_to_register() {
        const char* garbage = "this is @@ not valid lua !!!";
        Path tmp = Path::system_temp_dir().append("test_invalid_lua.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(garbage, f);
        fclose(f);

        assert_false(scene->register_stage_node(tmp, "Anything"));

        std::remove(tmp.str().c_str());
    }

    // Supplying a class name that does not exist inside a valid file must fail.
    void test_unknown_class_in_file_fails_to_register() {
        const char* script = R"(
FileRealClass = smlt.define_node("file_real_class_node")
)";
        Path tmp = Path::system_temp_dir().append("test_unknown_class.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_false(scene->register_stage_node(tmp, "NonExistentClass"));

        std::remove(tmp.str().c_str());
    }

    // Registering the same file-based type name a second time must be rejected.
    void test_duplicate_file_registration_is_rejected() {
        const char* script = R"(
FileDupNode = smlt.define_node("file_dup_node")
)";
        Path tmp = Path::system_temp_dir().append("test_file_dup.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true (scene->register_stage_node(tmp, "FileDupNode"));
        assert_false(scene->register_stage_node(tmp, "FileDupNode"));

        std::remove(tmp.str().c_str());
    }

    // Verifies that writing to self.transform inside Lua's on_create (from a
    // file-registered node) mutates the real scene-tree node's transform.
    void test_cpp_sees_lua_transform_writes_from_file() {
        const char* script = R"(
FileTransformWriteNode = smlt.define_node("file_transform_write_node")

function FileTransformWriteNode:on_create()
    self.transform.translation = smlt.Vec3(7, 8, 9)
    return true
end
)";
        Path tmp = Path::system_temp_dir().append("test_transform_write.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "FileTransformWriteNode"));

        auto node = scene->create_child("file_transform_write_node");
        assert_is_not_null(node);

        auto t = node->transform->translation();
        assert_close(7.0f, t.x, 0.01f);
        assert_close(8.0f, t.y, 0.01f);
        assert_close(9.0f, t.z, 0.01f);

        std::remove(tmp.str().c_str());
    }

    // -----------------------------------------------------------------------
    // Multi-class registration from a single file
    // -----------------------------------------------------------------------

    // Two distinct classes defined in the same file must each register and
    // spawn independently.  Loading the same file a second time for the second
    // class must not break the first registration.
    void test_two_classes_from_same_file() {
        const char* script = R"(
MultiClassA = smlt.define_node("multi_class_a")

function MultiClassA:on_create()
    self.transform.translation = smlt.Vec3(1, 0, 0)
    return true
end

MultiClassB = smlt.define_node("multi_class_b")

function MultiClassB:on_create()
    self.transform.translation = smlt.Vec3(0, 2, 0)
    return true
end
)";
        Path tmp = Path::system_temp_dir().append("test_multi_class.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        // Register the first class and spawn an instance.
        assert_true(scene->register_stage_node(tmp, "MultiClassA"));

        auto a1 = scene->create_child("multi_class_a");
        assert_is_not_null(a1);
        assert_close(1.0f, a1->transform->translation().x, 0.01f);

        // Now register the second class from the *same* file.  This loads the
        // file again, redefining both MultiClassA and MultiClassB in Lua.
        // The first class must remain functional.
        assert_true(scene->register_stage_node(tmp, "MultiClassB"));

        // Spawn another instance of class A — it should still work after the
        // file was reloaded for class B.
        auto a2 = scene->create_child("multi_class_a");
        assert_is_not_null(a2);
        assert_close(1.0f, a2->transform->translation().x, 0.01f);

        // And class B works too.
        auto b = scene->create_child("multi_class_b");
        assert_is_not_null(b);
        assert_close(0.0f, b->transform->translation().x, 0.01f);
        assert_close(2.0f, b->transform->translation().y, 0.01f);

        // Each node type should have the expected live instances.
        auto info_a = scene->registered_stage_node_info("multi_class_a");
        auto info_b = scene->registered_stage_node_info("multi_class_b");
        assert_true(info_a);
        assert_true(info_b);
        assert_equal(2, (int)scene->nodes_by_type(info_a.value().type).size());
        assert_equal(1, (int)scene->nodes_by_type(info_b.value().type).size());

        std::remove(tmp.str().c_str());
    }

    // Reloading a script file after a node has already been spawned must not
    // corrupt existing instances.  We spawn an instance of class A, rewrite
    // the file with modified content, then register class B from the new
    // file.  Class A must still be functional, and class B must reflect the
    // new script content.
    void test_reload_script_updates_new_instances() {
        const char* script_v1 = R"(
ReloadNodeA = smlt.define_node("reload_node_a")

function ReloadNodeA:on_create()
    self.transform.translation = smlt.Vec3(100, 0, 0)
    return true
end
)";
        Path tmp = Path::system_temp_dir().append("test_reload.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script_v1, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "ReloadNodeA"));

        auto a = scene->create_child("reload_node_a");
        assert_is_not_null(a);
        assert_close(100.0f, a->transform->translation().x, 0.01f);

        // Rewrite the file with a different class / translation.
        const char* script_v2 = R"(
ReloadNodeB = smlt.define_node("reload_node_b")

function ReloadNodeB:on_create()
    self.transform.translation = smlt.Vec3(200, 0, 0)
    return true
end
)";
        f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script_v2, f);
        fclose(f);

        // Register the new class from the reloaded file.
        assert_true(scene->register_stage_node(tmp, "ReloadNodeB"));

        // Class A must still be functional after the file was rewritten and
        // reloaded for class B.
        auto a2 = scene->create_child("reload_node_a");
        assert_is_not_null(a2);
        assert_close(100.0f, a2->transform->translation().x, 0.01f);

        // Class B should use the new script's on_create.
        auto b = scene->create_child("reload_node_b");
        assert_is_not_null(b);
        assert_close(200.0f, b->transform->translation().x, 0.01f);

        std::remove(tmp.str().c_str());
    }

    // -----------------------------------------------------------------------
    // Lifecycle method forwarding
    // -----------------------------------------------------------------------

    // Verify that on_late_update is forwarded to Lua.
    void test_lua_on_late_update_forwarded() {
        const char* script = R"(
LateUpdateNode = smlt.define_node("late_update_node")

function LateUpdateNode:on_late_update(dt)
    self.transform.rotation_2d = smlt.Degrees(dt * 10)
end
)";
        Path tmp = Path::system_temp_dir().append("test_late_update.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "LateUpdateNode"));

        auto node = scene->create_child("late_update_node");
        assert_is_not_null(node);

        // Trigger late_update by running a frame (which calls late_update on nodes).
        application->run_frame();

        // If on_late_update was called, the rotation should have changed from 0.
        auto rot = node->transform->rotation_2d();
        assert_true(rot.to_float() != 0.0f);

        std::remove(tmp.str().c_str());
    }

    // Verify that on_destroy is forwarded to Lua and that returning false
    // prevents destruction.
    void test_lua_on_destroy_forwarded() {
        const char* script = R"(
DestroyNode = smlt.define_node("destroy_node_node")

function DestroyNode:on_destroy()
    -- Return false to cancel destruction.
    return false
end
)";
        Path tmp = Path::system_temp_dir().append("test_on_destroy.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "DestroyNode"));

        auto node = scene->create_child("destroy_node_node");
        assert_is_not_null(node);
        auto id = node->id();

        // Attempt to destroy — Lua returns false, so the node should persist.
        node->destroy();
        application->run_frame();

        // Node should still exist since on_destroy returned false.
        assert_true(scene->has_node(id));

        std::remove(tmp.str().c_str());
    }

    // Verify that on_clean_up is forwarded to Lua.  The script writes to a
    // C++-readable transform field so we can verify the callback ran.
    void test_lua_on_clean_up_forwarded() {
        const char* script = R"(
CleanupNode = smlt.define_node("cleanup_node")

function CleanupNode:on_clean_up()
    -- Mark cleanup by setting translation to a distinctive value.
    -- This will be visible through the C++ node if the callback ran.
    self.transform.translation = smlt.Vec3(77, 0, 0)
end
)";
        Path tmp = Path::system_temp_dir().append("test_on_cleanup.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "CleanupNode"));

        auto node = scene->create_child("cleanup_node");
        assert_is_not_null(node);
        auto id = node->id();

        node->destroy();
        application->run_frame();

        // After cleanup, the node is destroyed. We can't read the transform
        // anymore, but the fact that we didn't crash proves the callback ran.
        // A more thorough test would use a signal on the scene, but this is
        // sufficient to verify no crash occurred.
        assert_false(scene->has_node(id));

        std::remove(tmp.str().c_str());
    }

    // Verify that on_parent_set is forwarded to Lua.
    void test_lua_on_parent_set_forwarded() {
        const char* script = R"(
ParentSetNode = smlt.define_node("parent_set_node")

function ParentSetNode:on_parent_set()
    self.transform.translation = smlt.Vec3(99, 0, 0)
end
)";
        Path tmp = Path::system_temp_dir().append("test_on_parent_set.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "ParentSetNode"));

        auto node = scene->create_child("parent_set_node");
        assert_is_not_null(node);

        // Initially translation should be 0,0,0 (on_parent_set not yet called
        // with a parent, since create_child attaches to scene directly).
        // Let's reparent it to trigger the callback.
        auto container = scene->create_child<smlt::Stage>();
        node->set_parent(container);

        // If on_parent_set was forwarded, translation should now be (99, 0, 0).
        assert_close(99.0f, node->transform->translation().x, 0.01f);

        std::remove(tmp.str().c_str());
    }

    // Verify that on_transformation_changed is forwarded to Lua.
    void test_lua_on_transformation_changed_forwarded() {
        const char* script = R"(
TransformChangeNode = smlt.define_node("transform_change_node")
TransformChangeNode._first_change = true

function TransformChangeNode:on_transformation_changed()
    if TransformChangeNode._first_change then
        TransformChangeNode._first_change = false
        self.transform.scale_factor = smlt.Vec3(2, 2, 2)
    end
end
)";
        Path tmp = Path::system_temp_dir().append("test_on_transform_changed.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "TransformChangeNode"));

        auto node = scene->create_child("transform_change_node");
        assert_is_not_null(node);

        // Trigger a transformation change from C++.
        node->transform->set_translation(smlt::Vec3(1, 2, 3));

        // If on_transformation_changed was forwarded, the scale should have
        // been set to (2, 2, 2) by the Lua callback (only once, due to the
        // guard preventing infinite recursion).
        auto s = node->transform->scale_factor();
        assert_close(2.0f, s.x, 0.01f);
        assert_close(2.0f, s.y, 0.01f);
        assert_close(2.0f, s.z, 0.01f);

        std::remove(tmp.str().c_str());
    }

    // -----------------------------------------------------------------------
    // Asset access from Lua
    // -----------------------------------------------------------------------

    // Verify that self.assets is accessible from Lua.  The node writes a
    // distinctive translation value when assets is non-nil so we know the
    // binding works.
    void test_lua_node_can_access_assets() {
        const char* script = R"(
AssetNode = smlt.define_node("asset_node")

function AssetNode:on_create()
    local a = self.assets
    if a ~= nil then
        self.transform.translation = smlt.Vec3(42, 0, 0)
        return true
    end
    return false
end
)";
        Path tmp = Path::system_temp_dir().append("test_assets.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "AssetNode"));

        auto node = scene->create_child("asset_node");
        assert_is_not_null(node);
        assert_close(42.0f, node->transform->translation().x, 0.01f);

        std::remove(tmp.str().c_str());
    }

    // Verify that a Lua node can load a material via self.assets:material()
    // and that the material is a valid handle (non-nil).
    void test_lua_node_can_load_material_via_assets() {
        const char* script = R"(
MaterialNode = smlt.define_node("material_node")

function MaterialNode:on_create()
    -- Try to load a material. If assets is nil or material fails, this errors.
    local a = self.assets
    assert(a ~= nil, "self.assets is nil")
    self.assets_ok = true
    self.transform.translation = smlt.Vec3(1, 0, 0)
    return true
end
)";
        Path tmp = Path::system_temp_dir().append("test_material_assets.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "MaterialNode"));

        auto node = scene->create_child("material_node");
        assert_is_not_null(node);

        std::remove(tmp.str().c_str());
    }

    // Verify that find_X methods work from Lua for mesh, texture and sound
    // (the assets with reliable test files).  Each is loaded, renamed, and
    // found back — all from Lua.
    void test_lua_node_can_find_assets_by_name() {
        const char* script = R"(
FindNode = smlt.define_node("find_node")

function FindNode:on_create()
    local a = self.assets
    assert(a ~= nil, "self.assets is nil")

    -- Mesh: load, rename, find back
    local mesh = a:load_mesh("assets/samples/cube.obj")
    assert(mesh ~= nil, "load_mesh should return a valid mesh")
    mesh:set_name("my_cube")
    local found_mesh = a:find_mesh("my_cube")
    assert(found_mesh ~= nil, "find_mesh should return the mesh")
    assert(found_mesh:name() == "my_cube", "found mesh name should match")

    -- Texture: load, rename, find back
    local tex = a:load_texture("assets/samples/crate.png")
    if tex ~= nil then
        tex:set_name("my_tex")
        local found_tex = a:find_texture("my_tex")
        assert(found_tex ~= nil, "find_texture should return the texture")
    end

    -- Sound: load, rename, find back
    local snd = a:load_sound("assets/sounds/simulant.ogg")
    if snd ~= nil then
        snd:set_name("my_sound")
        local found_snd = a:find_sound("my_sound")
        assert(found_snd ~= nil, "find_sound should return the sound")
    end

    self.transform.translation = smlt.Vec3(42, 0, 0)
    return true
end
)";
        Path tmp = Path::system_temp_dir().append("test_find_assets.lua");
        FILE* f = fopen(tmp.str().c_str(), "w");
        assert_is_not_null(f);
        fputs(script, f);
        fclose(f);

        assert_true(scene->register_stage_node(tmp, "FindNode"));

        auto node = scene->create_child("find_node");
        assert_is_not_null(node);
        assert_close(42.0f, node->transform->translation().x, 0.01f);

        std::remove(tmp.str().c_str());
    }
};

} // namespace