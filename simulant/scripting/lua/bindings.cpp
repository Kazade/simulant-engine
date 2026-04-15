#include "bindings.h"
#include "../../application.h"
#include "../../asset_manager.h"
#include "../../compositor.h"
#include "../../input/input_manager.h"
#include "../../nodes/stage_node.h"
#include "../../utils/hash/fnv1.h"
#include "../../window.h"
#include "interpreter.h"

#include <cstdint>

namespace smlt {

// Helper: convert a Lua table to Params
static Params lua_table_to_params(luabridge::LuaRef table) {
    Params params;
    if(table.isNil() || !table.isTable()) {
        return params;
    }
    
    table.push();
    lua_State* L = table.state();
    lua_pushnil(L);
    while(lua_next(L, -2) != 0) {
        if(lua_type(L, -2) == LUA_TSTRING) {
            std::string key = lua_tostring(L, -2);
            if(lua_isnumber(L, -1)) {
                double num = lua_tonumber(L, -1);
                if(num == (int)num) {
                    params.set(key, (int)num);
                } else {
                    params.set(key, (float)num);
                }
            } else if(lua_isboolean(L, -1)) {
                params.set(key, (bool)lua_toboolean(L, -1));
            } else if(lua_isstring(L, -1)) {
                params.set(key, std::string(lua_tostring(L, -1)));
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return params;
}

luabridge::LuaRef stage_node_meta(std::string name) {
    auto lua = smlt::get_app()->ensure_lua_ready();
    luabridge::LuaRef table = luabridge::newTable(lua->state_);
    table["name"] = name;
    table["node_type"] = smlt::fnv1<uint32_t>::hash(name.c_str());
    return table;
}

void lua_bind(lua_State* state) {
    luabridge::getGlobalNamespace(state)
        .beginNamespace("smlt")
        // ----------------------------------------------------------------
        // NodeParamType constants
        // ----------------------------------------------------------------
        .beginNamespace("NodeParamType")
        .addVariable("Invalid",          NODE_PARAM_TYPE_INVALID)
        .addVariable("Float",            NODE_PARAM_TYPE_FLOAT)
        .addVariable("FloatArray",       NODE_PARAM_TYPE_FLOAT_ARRAY)
        .addVariable("Int",              NODE_PARAM_TYPE_INT)
        .addVariable("IntArray",         NODE_PARAM_TYPE_INT_ARRAY)
        .addVariable("Bool",             NODE_PARAM_TYPE_BOOL)
        .addVariable("BoolArray",        NODE_PARAM_TYPE_BOOL_ARRAY)
        .addVariable("String",           NODE_PARAM_TYPE_STRING)
        .addVariable("MeshPtr",          NODE_PARAM_TYPE_MESH_PTR)
        .addVariable("TexturePtr",       NODE_PARAM_TYPE_TEXTURE_PTR)
        .addVariable("ParticleScriptPtr",NODE_PARAM_TYPE_PARTICLE_SCRIPT_PTR)
        .addVariable("StageNodePtr",     NODE_PARAM_TYPE_STAGE_NODE_PTR)
        .addVariable("PrefabPtr",        NODE_PARAM_TYPE_PREFAB_PTR)
        .addVariable("UIConfig",         NODE_PARAM_TYPE_UI_CONFIG)
        .addVariable("WidgetStylePtr",   NODE_PARAM_TYPE_WIDGET_STYLE_PTR)
        .addVariable("GeomCullerOpts",   NODE_PARAM_TYPE_GEOM_CULLER_OPTS)
        .addVariable("TextureFlags",     NODE_PARAM_TYPE_TEXTURE_FLAGS)
        .endNamespace()
        // ----------------------------------------------------------------
        // Scene (opaque handle, passed as a constructor argument)
        //
        //   scene.assets   -> AssetManager
        //   scene.input    -> InputManager
        // ----------------------------------------------------------------
        .beginClass<Scene>("Scene")
        .addProperty("assets", &Scene::assets)
        .addProperty("input", &Scene::input)
        .addProperty("compositor", &Scene::compositor)
        .addProperty("app", &Scene::app)
        .addProperty("window", &Scene::window)
        .endClass()
        // ----------------------------------------------------------------
        // SceneCompositor — manages scene layers and rendering.
        // ----------------------------------------------------------------
        .beginClass<SceneCompositor>("SceneCompositor")
        .endClass()
        // ----------------------------------------------------------------
        // Application — the running application instance.
        // ----------------------------------------------------------------
        .beginClass<Application>("Application")
        .endClass()
        // ----------------------------------------------------------------
        // Window — the rendering window.
        // ----------------------------------------------------------------
        .beginClass<Window>("Window")
        .endClass()
        // ----------------------------------------------------------------
        // AssetManager — access to loaded assets.
        //
        //   local mesh   = scene.assets:mesh(id)
        //   local tex    = scene.assets:texture(id)
        //   local mat    = scene.assets:material(id)
        //   local sound  = scene.assets:sound(id)
        //   local font   = scene.assets:font(id)
        //   local prefab = scene.assets:prefab(id)
        // ----------------------------------------------------------------
        .beginClass<AssetManager>("AssetManager")
        // --- load methods (path-only convenience overloads) ---
        .addFunction("load_mesh",        [](AssetManager* am, const std::string& path) { return am->load_mesh(path); })
        .addFunction("load_texture",     [](AssetManager* am, const std::string& path) { return am->load_texture(path); })
        .addFunction("load_material",    [](AssetManager* am, const std::string& path) { return am->load_material(path); })
        .addFunction("load_sound",       [](AssetManager* am, const std::string& path) { return am->load_sound(path); })
        .addFunction("load_font",        [](AssetManager* am, const std::string& path) { return am->load_font(path); })
        .addFunction("load_prefab",      [](AssetManager* am, const std::string& path) { return am->load_prefab(path); })
        // --- fetch by numeric ID ---
        .addFunction("mesh",           static_cast<MeshPtr (AssetManager::*)(AssetID)>(&AssetManager::mesh))
        .addFunction("texture",        static_cast<TexturePtr (AssetManager::*)(AssetID)>(&AssetManager::texture))
        .addFunction("material",       static_cast<MaterialPtr (AssetManager::*)(const AssetID&)>(&AssetManager::material))
        .addFunction("sound",          static_cast<SoundPtr (AssetManager::*)(AssetID)>(&AssetManager::sound))
        .addFunction("font",           static_cast<FontPtr (AssetManager::*)(AssetID)>(&AssetManager::font))
        .addFunction("prefab",         static_cast<PrefabPtr (AssetManager::*)(AssetID)>(&AssetManager::prefab))
        // --- find by name ---
        .addFunction("find_mesh",      &AssetManager::find_mesh)
        .addFunction("find_texture",   &AssetManager::find_texture)
        .addFunction("find_material",  &AssetManager::find_material)
        .addFunction("find_sound",     &AssetManager::find_sound)
        .addFunction("find_font",      &AssetManager::find_font)
        .addFunction("find_prefab",    &AssetManager::find_prefab)
        // --- presence checks ---
        .addFunction("has_mesh",       &AssetManager::has_mesh)
        .addFunction("has_texture",    &AssetManager::has_texture)
        .addFunction("has_material",   &AssetManager::has_material)
        .addFunction("has_sound",      &AssetManager::has_sound)
        .addFunction("has_font",       &AssetManager::has_font)
        .addFunction("has_prefab",     &AssetManager::has_prefab)
        // --- counts ---
        .addFunction("mesh_count",     &AssetManager::mesh_count)
        .addFunction("texture_count",  &AssetManager::texture_count)
        .addFunction("material_count", &AssetManager::material_count)
        .addFunction("sound_count",    &AssetManager::sound_count)
        .addFunction("font_count",     &AssetManager::font_count)
        .addFunction("prefab_count",   &AssetManager::prefab_count)
        .endClass()
        // ----------------------------------------------------------------
        // Asset base class — exposes name(), has_name(), set_name().
        // Every asset type below inherits from this, but LuaBridge does not
        // handle virtual-base inheritance so we repeat the bindings on each
        // concrete class.
        // ----------------------------------------------------------------
        #define SMLT_BIND_NAMEABLE(klass) \
            .addFunction("name",     [](klass* a) { return a->name(); }) \
            .addFunction("has_name", [](klass* a) { return a->has_name(); }) \
            .addFunction("set_name", [](klass* a, const std::string& n) { a->set_name(n); })
        // ----------------------------------------------------------------
        // Mesh — 3D geometry asset.
        // ----------------------------------------------------------------
        .beginClass<Mesh>("Mesh")
        SMLT_BIND_NAMEABLE(Mesh)
        .endClass()
        // ----------------------------------------------------------------
        // Texture — image data asset.
        // ----------------------------------------------------------------
        .beginClass<Texture>("Texture")
        SMLT_BIND_NAMEABLE(Texture)
        .endClass()
        // ----------------------------------------------------------------
        // Material — shader + parameter configuration asset.
        // ----------------------------------------------------------------
        .beginClass<Material>("Material")
        SMLT_BIND_NAMEABLE(Material)
        .endClass()
        // ----------------------------------------------------------------
        // Sound — audio data asset.
        // ----------------------------------------------------------------
        .beginClass<Sound>("Sound")
        SMLT_BIND_NAMEABLE(Sound)
        .endClass()
        // ----------------------------------------------------------------
        // Font — font data for text rendering.
        // ----------------------------------------------------------------
        .beginClass<Font>("Font")
        SMLT_BIND_NAMEABLE(Font)
        .endClass()
        // ----------------------------------------------------------------
        // Prefab — reusable scene subtree asset.
        // ----------------------------------------------------------------
        .beginClass<Prefab>("Prefab")
        SMLT_BIND_NAMEABLE(Prefab)
        .endClass()
        #undef SMLT_BIND_NAMEABLE
        // ----------------------------------------------------------------
        // InputManager — access to input state and axes.
        //
        //   local val = scene.input:axis_value("fire")
        //   local pressed = scene.input:axis_was_pressed("jump")
        // ----------------------------------------------------------------
        .beginClass<InputManager>("InputManager")
        .addFunction("axis_value",       &InputManager::axis_value)
        .addFunction("axis_was_pressed", &InputManager::axis_was_pressed)
        .addFunction("axis_was_released",&InputManager::axis_was_released)
        .endClass()
        // ----------------------------------------------------------------
        // NodeParam (opaque, used in parameter sets)
        // ----------------------------------------------------------------
        .beginClass<NodeParam>("NodeParam")
        .endClass()
        // ----------------------------------------------------------------
        // Degrees — wraps a float angle value.
        //
        //   local d = smlt.Degrees(45.0)
        //   print(d:to_float())   -- 45.0
        // ----------------------------------------------------------------
        .beginClass<Degrees>("Degrees")
        .addConstructor<void(*)(float)>()
        .addFunction("to_float", &Degrees::to_float)
        .endClass()
        // ----------------------------------------------------------------
        // Vec2 — 2-D floating-point vector.
        //
        //   local v = smlt.Vec2(x, y)   or   smlt.Vec2()  (zero)
        //
        //   Properties (read-write): x, y
        //   Methods: length, length_squared, normalize, normalized,
        //            dot, lerp
        // ----------------------------------------------------------------
        .beginClass<Vec2>("Vec2")
        .addConstructor<void(*)(), void(*)(float, float)>()
        .addProperty("x", &Vec2::x, &Vec2::x)
        .addProperty("y", &Vec2::y, &Vec2::y)
        .addFunction("length",         &Vec2::length)
        .addFunction("length_squared", &Vec2::length_squared)
        .addFunction("normalize",      &Vec2::normalize)
        .addFunction("normalized",     &Vec2::normalized)
        .addFunction("dot",            &Vec2::dot)
        .addFunction("lerp",           &Vec2::lerp)
        .endClass()
        // ----------------------------------------------------------------
        // Vec3 — 3-D floating-point vector.
        //
        //   local v = smlt.Vec3(x, y, z)
        //            smlt.Vec3(s)        -- fills all three components
        //            smlt.Vec3()         -- zero vector
        //
        //   Properties (read-write): x, y, z
        //   Methods: length, length_squared, normalize, normalized,
        //            dot, cross, lerp, distance_to
        //   Statics: Vec3.up, Vec3.down, Vec3.left, Vec3.right,
        //            Vec3.forward, Vec3.backward, Vec3.zero, Vec3.one
        // ----------------------------------------------------------------
        .beginClass<Vec3>("Vec3")
        .addConstructor<void(*)(), void(*)(float), void(*)(float, float, float)>()
        .addProperty("x", &Vec3::x, &Vec3::x)
        .addProperty("y", &Vec3::y, &Vec3::y)
        .addProperty("z", &Vec3::z, &Vec3::z)
        .addFunction("length",         &Vec3::length)
        .addFunction("length_squared", &Vec3::length_squared)
        .addFunction("normalize",      &Vec3::normalize)
        .addFunction("normalized",
            [](const Vec3* self) -> Vec3 { return self->normalized(); })
        .addFunction("dot",            &Vec3::dot)
        .addFunction("cross",          &Vec3::cross)
        .addFunction("lerp",           &Vec3::lerp)
        .addFunction("distance_to",
            static_cast<float (Vec3::*)(const Vec3&) const>(&Vec3::distance_to))
        .addStaticFunction("up",       &Vec3::up)
        .addStaticFunction("down",     &Vec3::down)
        .addStaticFunction("left",     &Vec3::left)
        .addStaticFunction("right",    &Vec3::right)
        .addStaticFunction("forward",  &Vec3::forward)
        .addStaticFunction("backward", &Vec3::backward)
        .addStaticFunction("zero",     &Vec3::zero)
        .addStaticFunction("one",      &Vec3::one)
        .endClass()
        // ----------------------------------------------------------------
        // Quaternion — unit quaternion representing an orientation.
        //
        //   smlt.Quaternion()                              -- identity
        //   smlt.Quaternion(x, y, z, w)                   -- explicit xyzw
        //   smlt.Quaternion(axis_vec3, degrees)            -- axis-angle
        //   smlt.Quaternion(pitch_deg, yaw_deg, roll_deg)  -- Euler angles
        //
        //   Properties (read-write): x, y, z, w
        //   Methods: normalized, forward, up, right, slerp, nlerp
        // ----------------------------------------------------------------
        .beginClass<Quaternion>("Quaternion")
        .addConstructor<
            void(*)(),
            void(*)(float, float, float, float),
            void(*)(const Vec3&, const Degrees&),
            void(*)(const Degrees&, const Degrees&, const Degrees&)>()
        .addProperty("x", &Quaternion::x, &Quaternion::x)
        .addProperty("y", &Quaternion::y, &Quaternion::y)
        .addProperty("z", &Quaternion::z, &Quaternion::z)
        .addProperty("w", &Quaternion::w, &Quaternion::w)
        .addFunction("normalized",
            [](const Quaternion* self) -> Quaternion { return self->normalized(); })
        .addFunction("forward",    &Quaternion::forward)
        .addFunction("up",         &Quaternion::up)
        .addFunction("right",      &Quaternion::right)
        .addFunction("slerp",      &Quaternion::slerp)
        .addFunction("nlerp",      &Quaternion::nlerp)
        .endClass()
        // ----------------------------------------------------------------
        // Transform — the spatial transform attached to every StageNode.
        //
        // Never constructed directly from Lua; obtain it via node.transform.
        //
        // World-space (absolute) properties:
        //   t.position      = smlt.Vec3(...)
        //   t.orientation   = smlt.Quaternion(...)
        //   t.scale         = smlt.Vec3(...)
        //
        // Parent-relative (local) properties:
        //   t.translation   = smlt.Vec3(...)
        //   t.rotation      = smlt.Quaternion(...)
        //   t.scale_factor  = smlt.Vec3(...)
        //
        // Direction helpers (derived from orientation, read-only via method):
        //   t:forward()    t:up()    t:right()
        //
        // Mutation helpers:
        //   t:translate(vec3)
        //   t:rotate(quat)
        //   t:rotate(axis_vec3, degrees)
        //   t:rotate(pitch_deg, yaw_deg, roll_deg)
        //   t:scale_by(vec3)  /  t:scale_by(float)
        //   t:look_at(target_vec3)
        //   t:look_at(target_vec3, up_vec3)
        //
        // 2-D helpers (for 2-D scenes):
        //   t.position_2d    = smlt.Vec2(...)
        //   t.translation_2d = smlt.Vec2(...)
        //   t.orientation_2d = smlt.Degrees(...)
        //   t.rotation_2d    = smlt.Degrees(...)
        //   t:set_scale_factor_2d(vec2)
        //   t:translate_2d(vec2)
        //   t:rotate_2d(degrees)
        // ----------------------------------------------------------------
        .beginClass<Transform>("Transform")
        // World-space properties
        .addProperty("position",
            &Transform::position,      &Transform::set_position)
        .addProperty("orientation",
            &Transform::orientation,   &Transform::set_orientation)
        .addProperty("scale",
            &Transform::scale,         &Transform::set_scale)
        // Parent-relative (local) properties
        .addProperty("translation",
            &Transform::translation,   &Transform::set_translation)
        .addProperty("rotation",
            &Transform::rotation,      &Transform::set_rotation)
        .addProperty("scale_factor",
            &Transform::scale_factor,  &Transform::set_scale_factor)
        // Direction helpers (read-only, derived from world orientation)
        .addFunction("forward", &Transform::forward)
        .addFunction("up",      &Transform::up)
        .addFunction("right",   &Transform::right)
        // Mutation helpers
        .addFunction("translate",
            static_cast<void (Transform::*)(const Vec3&)>(&Transform::translate))
        .addFunction("rotate",
            static_cast<void (Transform::*)(const Quaternion&)>(
                &Transform::rotate),
            static_cast<void (Transform::*)(const Vec3&, const Degrees&)>(
                &Transform::rotate),
            static_cast<void (Transform::*)(const Degrees&, const Degrees&, const Degrees&)>(
                &Transform::rotate))
        .addFunction("scale_by",
            static_cast<void (Transform::*)(const Vec3&)>(&Transform::scale_by),
            static_cast<void (Transform::*)(float)>(&Transform::scale_by))
        // look_at has a default parameter for `up`; expose both arities as
        // overloads via lambdas so Lua callers can omit the up vector.
        .addFunction("look_at",
            [](Transform* t, const Vec3& target) {
                t->look_at(target);
            },
            [](Transform* t, const Vec3& target, const Vec3& up) {
                t->look_at(target, up);
            })
        // 2-D helpers
        .addProperty("position_2d",
            &Transform::position_2d,    &Transform::set_position_2d)
        .addProperty("translation_2d",
            &Transform::translation_2d, &Transform::set_translation_2d)
        .addProperty("orientation_2d",
            &Transform::orientation_2d, &Transform::set_orientation_2d)
        .addProperty("rotation_2d",
            &Transform::rotation_2d,    &Transform::set_rotation_2d)
        .addFunction("set_scale_factor_2d", &Transform::set_scale_factor_2d)
        .addFunction("translate_2d",        &Transform::translate_2d)
        .addFunction("rotate_2d",           &Transform::rotate_2d)
        .endClass()
        // ----------------------------------------------------------------
        // Utility functions
        // ----------------------------------------------------------------
        .addFunction("stage_node_meta", &stage_node_meta)
        // ----------------------------------------------------------------
        // LuaStageNode — exposed to Lua as smlt.StageNode.
        //
        // Each Lua node class created with smlt.define_node() gets a plain
        // Lua wrapper table whose __index metamethod delegates unknown key
        // lookups to a _cpp_node field.  That field is a UserdataPtr (a
        // non-owning raw-pointer userdata) pointing at the real slab-allocated
        // LuaStageNode that lives in the scene tree.
        //
        // NO addDestructor is registered here.  Application::~Application()
        // tears down scripting interpreters (lua_close) *before* destroying
        // scenes, so the Lua GC runs while scene nodes are still alive.
        // Without addDestructor, gc_metamethod only calls ~UserdataPtr(),
        // which frees the LuaBridge wrapper object on the Lua heap without
        // touching the slab-allocated node — which is exactly what we want.
        // Registering a destructor here would cause it to fire on the live
        // _cpp_node pointer during lua_close GC, calling clean_up() on a node
        // that is mid-teardown and may be re-entered or already invalid.
        // ----------------------------------------------------------------
        .beginClass<LuaStageNode>("StageNode")
        .addConstructor<void (*)(Scene*, StageNodeType, std::string,
                                 std::set<NodeParam>)>()
        .addProperty("scene",     &LuaStageNode::lua_get_scene)
        .addProperty("node_type", &LuaStageNode::lua_get_node_type)
        .addProperty("transform", &LuaStageNode::lua_get_transform)
        .addProperty("assets",    &LuaStageNode::lua_get_assets)
        .addFunction("create_child", [](LuaStageNode* node, const char* name, luabridge::LuaRef params_table) -> StageNode* {
            return node->create_child(name, lua_table_to_params(params_table));
        })
        .addFunction("create_mixin", [](LuaStageNode* node, const std::string& name, luabridge::LuaRef params_table) -> StageNode* {
            return node->create_mixin(name, lua_table_to_params(params_table));
        })
        .endClass();
}
} // namespace smlt
