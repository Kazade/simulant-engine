#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/scripting/lua/interpreter.h"
#include "simulant/nodes/ui/button.h"
#include "simulant/nodes/ui/frame.h"

namespace {

using namespace smlt;

// A minimal native (C++) stage node used by the registry test.
class RegistryTestNode: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META("registry_test_node", smlt::STAGE_NODE_USAGE_NODE_ONLY);

    RegistryTestNode(Scene* owner):
        StageNode(owner, Meta::node_type) {}
};

// ---------------------------------------------------------------------------
// Lua script fixtures
// ---------------------------------------------------------------------------

// on_update / on_fixed_update write counters into the scene transform so the
// C++ test can observe that both hooks were forwarded.
const char* update_scene_script = R"(
UpdateScene = smlt.define_scene("update_scene")

function UpdateScene:on_load()
    self.updates = 0
    self.fixed_updates = 0
end

function UpdateScene:on_update(dt)
    self.updates = self.updates + 1
    self.transform.translation = smlt.Vec3(self.updates, self.fixed_updates, 0)
end

function UpdateScene:on_fixed_update(step)
    self.fixed_updates = self.fixed_updates + 1
    self.transform.translation = smlt.Vec3(self.updates, self.fixed_updates, 0)
end
)";

// Reads the activation load args through Scene:load_arg_* and encodes them
// into the transform.
const char* load_arg_scene_script = R"(
LoadArgScene = smlt.define_scene("load_arg_scene")

function LoadArgScene:on_load()
    local n = self._cpp_node:load_arg_count()
    local i = self._cpp_node:load_arg_int(0)
    local f = self._cpp_node:load_arg_float(1)
    local s = self._cpp_node:load_arg_string(2)
    local str_ok = (s == "hello") and 1 or 0
    self.transform.translation = smlt.Vec3(n, i, f + str_ok)
end
)";

// A node with a method and a field; the proxy scene below exercises the
// __index/__newindex fallbacks by calling them directly on the node handle.
const char* counter_node_script = R"(
CounterNode = smlt.define_node("counter_node")

function CounterNode:on_create()
    self.value = 10
    return true
end

function CounterNode:bump(n)
    self.value = self.value + n
end

function CounterNode:get_value()
    return self.value
end
)";

// Creates a CounterNode child and calls its method / reads and writes its
// field with ordinary syntax. value ends up 10 + 5 + 1 = 16.
const char* proxy_scene_script = R"(
ProxyScene = smlt.define_scene("proxy_scene")

function ProxyScene:on_load()
    local child = smlt.create_child_node(self, "counter_node", {})
    child:bump(5)
    child.value = child.value + 1
    self.transform.translation = smlt.Vec3(child:get_value(), 0, 0)
end
)";

class LuaBindingTests: public test::SimulantTestCase {
public:
    void test_scene_load_args_forwarded_to_lua() {
        assert_true(application->scenes->register_scene(
            load_arg_scene_script, "LoadArgScene"));

        application->scenes->preload("load_arg_scene", 7, 2.5f,
                                     std::string("hello"));

        auto lua_scene = application->scenes->resolve_scene("load_arg_scene");
        auto t = lua_scene->transform->translation();
        assert_close(3.0f, t.x, 0.01f); // three args
        assert_close(7.0f, t.y, 0.01f); // int arg
        assert_close(3.5f, t.z, 0.01f); // float 2.5 + string check
    }

    void test_lua_scene_update_hooks_forwarded() {
        assert_true(application->scenes->register_scene(
            update_scene_script, "UpdateScene"));

        auto lua_scene = application->scenes->resolve_scene("update_scene");
        lua_scene->load();

        auto* lua = dynamic_cast<LuaScene*>(lua_scene.get());
        assert_is_not_null(lua);

        lua->on_update(0.1f);
        lua->on_fixed_update(0.1f);

        auto t = lua_scene->transform->translation();
        assert_close(1.0f, t.x, 0.01f);
        assert_close(1.0f, t.y, 0.01f);
    }

    void test_lua_index_newindex_proxy() {
        assert_true(application->scenes->register_scene(
            proxy_scene_script, "ProxyScene"));

        auto lua_scene = application->scenes->resolve_scene("proxy_scene");
        assert_true(lua_scene->register_stage_node(counter_node_script,
                                                   "CounterNode"));
        lua_scene->load();

        auto t = lua_scene->transform->translation();
        assert_close(16.0f, t.x, 0.01f);
    }

    void test_native_stage_node_registry() {
        register_native_stage_node_type("registry_test_node",
                                        [](StageNodeManager* m) {
            return m->register_stage_node<RegistryTestNode>();
        });

        assert_true(register_native_stage_node_type_on(scene,
                                                       "registry_test_node"));
        auto* node = scene->create_child("registry_test_node");
        assert_is_not_null(node);
    }

    void test_native_stage_node_unknown_name_fails() {
        assert_false(register_native_stage_node_type_on(scene,
                                                        "no_such_node"));
    }

    void test_texture_set_pixel() {
        auto tex = scene->assets->create_texture(4, 4);
        tex->set_pixel(0, 0, 255, 128, 0, 255);

        auto* data = tex->data();
        assert_equal((uint8_t)255, data[0]);
        assert_equal((uint8_t)128, data[1]);
        assert_equal((uint8_t)0, data[2]);
        assert_equal((uint8_t)255, data[3]);
    }

    void test_frame_drops_destroyed_child() {
        auto* frame = scene->create_child<ui::Frame>();
        auto* button = scene->create_child<ui::Button>(std::string(""));

        frame->pack_child(button);
        assert_equal(1, (int)frame->packed_children().size());

        button->destroy();
        assert_equal(0, (int)frame->packed_children().size());
    }

    void test_input_state_sentinel_ids_are_safe() {
        auto* input = window->input_state.get();

        assert_false(input->keyboard_key_state(ALL_KEYBOARDS,
                                               KEYBOARD_CODE_A));
        assert_false(input->mouse_button_state(ALL_MICE, 0));

        auto p = input->mouse_position(ALL_MICE);
        assert_close(0.0f, p.x, 0.001f);
        assert_close(0.0f, p.y, 0.001f);
    }
};

} // namespace
