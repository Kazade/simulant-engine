#include "bindings.h"
#include "../../application.h"
#include "../../asset_manager.h"
#include "../../color.h"
#include "../../compositor.h"
#include "../../input/input_manager.h"
#include "../../keycodes.h"
#include "../../math/plane.h"
#include "../../math/ray.h"
#include "../../nodes/camera.h"
#include "../../nodes/stage_node.h"
#include "../../nodes/ui/button.h"
#include "../../nodes/ui/frame.h"
#include "../../nodes/ui/image.h"
#include "../../nodes/ui/label.h"
#include "../../nodes/ui/progress_bar.h"
#include "../../nodes/ui/ui_manager.h"
#include "../../nodes/ui/widget.h"
#include "../../texture.h"
#include "../../utils/hash/fnv1.h"
#include "../../window.h"
#include "interpreter.h"

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <vector>

namespace smlt {

namespace {

// A Lua function held by registry reference rather than a LuaRef, so that
// destroying the callback after lua_close() (which Application does before
// tearing down scenes) is harmless: the destructor only forgets an integer.
// The registry entry is intentionally never unref'd - it is reclaimed en
// masse when the state closes.
struct LuaCallback {
    lua_State* L = nullptr;
    int ref = LUA_NOREF;

    LuaCallback() = default;
    LuaCallback(lua_State* state, int r) :
        L(state), ref(r) {}

    void operator()() {
        if(!L || ref == LUA_NOREF) {
            return;
        }
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        if(lua_pcall(L, 0, 0, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            S_ERROR("Lua callback error: {0}", err ? err : "(unknown)");
            lua_pop(L, 1);
        }
    }
};

// Converts a Lua function into a shared LuaCallback. Returns nullptr if the
// argument isn't a function.
std::shared_ptr<LuaCallback> make_lua_callback(luabridge::LuaRef fn) {
    if(!fn.isFunction()) {
        return nullptr;
    }
    lua_State* L = fn.state();
    fn.push(L);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    return std::make_shared<LuaCallback>(L, ref);
}

std::vector<LuaBindingHook>& lua_binding_hooks() {
    static std::vector<LuaBindingHook> hooks;
    return hooks;
}

// __index / __newindex fallbacks for the StageNode class. Any key not found
// as a bound method/property is forwarded to the node's Lua script instance
// (when it's a LuaStageNode), so Lua-authored methods and fields work on a
// node handle with ordinary syntax: node:foo(), node.bar, node.baz = 1.
// Built-in nodes (Actor, Frame, ...) fall through to nil.
luabridge::LuaRef stage_node_index_fallback(StageNode& self,
                                              const luabridge::LuaRef& key,
                                              lua_State* L) {
    auto* ln = dynamic_cast<LuaStageNode*>(&self);
    if(!ln) {
        return luabridge::LuaRef(L);
    }
    return ln->lua_index(key);
}

luabridge::LuaRef stage_node_newindex_fallback(StageNode& self,
                                               const luabridge::LuaRef& key,
                                               const luabridge::LuaRef& value,
                                               lua_State* L) {
    auto* ln = dynamic_cast<LuaStageNode*>(&self);
    if(!ln) {
        return luabridge::LuaRef(L);
    }
    ln->lua_newindex(key, value);
    return luabridge::LuaRef(L);
}

} // namespace

void register_lua_binding_hook(LuaBindingHook hook) {
    lua_binding_hooks().push_back(std::move(hook));
}

// Helper: convert a Lua table to Params
//
// Handles the value shapes Lua scripts actually pass:
//   - number/boolean/string -> int-or-float/bool/string.
//   - table (e.g. {x, y, z}) -> FloatArray, so the built-in position/
//     orientation/scale/translation/rotation/scale_factor params every
//     stage node accepts (see get_node_params() in nodes/stage_node.h) can
//     actually be set via create_child_node()/create_mixin().
//   - userdata wrapping a Mesh/Texture asset (e.g. the result of
//     AssetManager:load_mesh()) -> MeshRef/TextureRef, so e.g.
//     create_child_node(self, "actor", {mesh = mesh}) can populate the
//     "mesh" param an Actor requires.
static Params lua_table_to_params(luabridge::LuaRef table) {
    Params params;
    if(table.isNil() || !table.isTable()) {
        return params;
    }

    for(auto [key, val] : luabridge::pairs(table)) {
        if(!key.isString()) {
            continue;
        }
        std::string k = key.cast<std::string>().valueOr("");
        if(k.empty()) {
            continue;
        }

        if(val.isNumber()) {
            double num = val.cast<double>().valueOr(0.0);
            if(num == (int)num) {
                params.set(k, (int)num);
            } else {
                params.set(k, (float)num);
            }
        } else if(val.isBool()) {
            params.set(k, val.cast<bool>().valueOr(false));
        } else if(val.isString()) {
            params.set(k, val.cast<std::string>().valueOr(""));
        } else if(val.isTable()) {
            FloatArray arr;
            for(int i = 1;; ++i) {
                luabridge::LuaRef item = val[i];
                if(item.isNil()) {
                    break;
                }
                arr.push_back(item.cast<float>().valueOr(0.0f));
            }
            params.set(k, arr);
        } else if(val.isUserdata()) {
            auto mesh = val.cast<MeshPtr>();
            if(mesh) {
                params.set(k, MeshRef(*mesh));
                continue;
            }
            auto texture = val.cast<TexturePtr>();
            if(texture) {
                params.set(k, TextureRef(*texture));
                continue;
            }
        }
    }

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
    using ui::Px;
    using ui::UICoord;
    using ui::TextAlignment;
    using ui::LayoutDirection;

    luabridge::getGlobalNamespace(state)
        .beginNamespace("smlt")
        // ----------------------------------------------------------------
        // Render layer priorities / buffer clear flags
        // ----------------------------------------------------------------
        .addVariable("RENDER_PRIORITY_MAIN", (int)RENDER_PRIORITY_MAIN)
        .addVariable("RENDER_PRIORITY_FOREGROUND", (int)RENDER_PRIORITY_FOREGROUND)
        .addVariable("RENDER_PRIORITY_BACKGROUND", (int)RENDER_PRIORITY_BACKGROUND)
        .addVariable("BUFFER_CLEAR_ALL", (int)BUFFER_CLEAR_ALL)
        .addVariable("BUFFER_CLEAR_COLOR", (int)BUFFER_CLEAR_COLOR_BUFFER)
        .addVariable("BUFFER_CLEAR_DEPTH", (int)BUFFER_CLEAR_DEPTH_BUFFER)
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
        // StageNode — generic non-owning handle for a node in the scene
        // tree. This is what create_child()/create_mixin() (below, on both
        // Scene and LuaStageNode) return: it exists so LuaBridge has a
        // registered class to marshal StageNode* values through, whether
        // the concrete node is a built-in C++ type (Stage, Camera3D, ...)
        // or a Lua-authored node created via smlt.define_node() (see
        // LuaStageNode, which derives from this class below so Lua nodes
        // get destroy()/is_destroyed() for free too).
        //
        //   node.transform
        //   node:destroy()
        //   node:is_destroyed()
        // ----------------------------------------------------------------
        .beginClass<StageNode>("StageNode")
        .addProperty("transform", &StageNode::get_transform)
        .addFunction("destroy", [](StageNode* n) { return n->destroy(); })
        .addFunction("is_destroyed", [](StageNode* n) { return n->is_destroyed(); })
        .addFunction("name", [](StageNode* n) { return n->name(); })
        .addFunction("set_name", [](StageNode* n, const std::string& name) { n->set_name(name); })
        .addFunction("is_visible", [](StageNode* n) { return n->is_visible(); })
        .addFunction("set_visible", [](StageNode* n, bool v) { n->set_visible(v); })
        .addFunction("aabb_min", [](StageNode* n) { return n->transformed_aabb().min(); })
        .addFunction("aabb_max", [](StageNode* n) { return n->transformed_aabb().max(); })
        .addFunction("find_descendent_with_name",
            [](StageNode* n, const std::string& name) -> StageNode* {
            return n->find_descendent_with_name(name);
        })
        .addFunction("adopt_children", [](StageNode* n, StageNode* child) {
            n->adopt_children(child);
        })
        .addFunction("children", [](StageNode* n) -> luabridge::LuaRef {
            lua_State* L = smlt::get_app()->ensure_lua_ready()->lua_state();
            luabridge::LuaRef out = luabridge::newTable(L);
            int i = 1;
            for(auto& c: n->each_child()) {
                out[i++] = &c;
            }
            return out;
        })
        // Convenience transform wrappers. The `transform` property works for
        // nodes whose registered class is StageNode itself or a direct
        // deriveClass chain, but is unreliable for some widget subclasses, so
        // these explicit methods are the dependable path for Lua UI code.
        .addFunction("set_position_2d", [](StageNode* n, float x, float y) {
            n->get_transform()->set_position_2d(Vec2(x, y));
        })
        .addFunction("position_2d", [](StageNode* n) {
            return n->get_transform()->position_2d();
        })
        .addFunction("set_position", [](StageNode* n, float x, float y, float z) {
            n->get_transform()->set_position(Vec3(x, y, z));
        })
        .addFunction("position", [](StageNode* n) {
            return n->get_transform()->position();
        })
        .addFunction("translate", [](StageNode* n, float x, float y, float z) {
            n->get_transform()->translate(Vec3(x, y, z));
        })
        .addFunction("look_at", [](StageNode* n, float x, float y, float z) {
            n->get_transform()->look_at(Vec3(x, y, z));
        })
        .addFunction("set_scale", [](StageNode* n, float x, float y, float z) {
            n->get_transform()->set_scale(Vec3(x, y, z));
        })
        .addFunction("set_orientation_2d", [](StageNode* n, float degrees) {
            n->get_transform()->set_orientation_2d(Degrees(degrees));
        })
        // Render ordering within a layer (higher = drawn later/on top).
        .addFunction("set_render_priority", [](StageNode* n, int priority) {
            n->set_render_priority((RenderPriority)priority);
        })
        .addFunction("render_priority", [](StageNode* n) {
            return (int)n->render_priority();
        })
        // Returns true if this node's script defines the named method.
        .addFunction("has_lua_method", [](StageNode* n, const char* name) {
            auto* ln = dynamic_cast<LuaStageNode*>(n);
            return ln ? ln->has_lua_method(name) : false;
        })
        // Child creation from a node handle. Nodes created via
        // create_child() come back as StageNode*, so dynamic_cast to
        // LuaStageNode to reach its create_child/create_mixin.
        .addFunction("create_child", [](StageNode* n, const char* name, luabridge::LuaRef params_table) -> StageNode* {
            auto* ln = dynamic_cast<LuaStageNode*>(n);
            return ln ? ln->create_child(name, lua_table_to_params(params_table)) : nullptr;
        })
        .addFunction("create_mixin", [](StageNode* n, const std::string& name, luabridge::LuaRef params_table) -> StageNode* {
            auto* ln = dynamic_cast<LuaStageNode*>(n);
            return ln ? ln->create_mixin(name, lua_table_to_params(params_table)) : nullptr;
        })
        // Forward unknown keys to the Lua script instance (see the fallback
        // functions above), so Lua-authored methods/fields work on node
        // handles.
        .addIndexMetaMethod(&stage_node_index_fallback)
        .addNewIndexMetaMethod(&stage_node_newindex_fallback)
        // UI widget factories live on StageNode (dynamically dispatched to
        // the UIManager it points at) rather than on the UIManager class
        // itself: LuaBridge doesn't adjust pointers for multiple
        // inheritance, and UIManager's first base is EventListener, so a
        // UIManager* passed as a StageNode* would point at the wrong
        // subobject. Returning the UIManager as a StageNode* from the start
        // sidesteps that entirely.
        .addFunction("create_label", [](StageNode* n) -> ui::Label* {
            auto* m = dynamic_cast<ui::UIManager*>(n);
            // "text" is a required param, so pass an empty string.
            return m ? m->create_child<ui::Label>(std::string("")) : nullptr;
        })
        .addFunction("create_button", [](StageNode* n) -> ui::Button* {
            auto* m = dynamic_cast<ui::UIManager*>(n);
            return m ? m->create_child<ui::Button>(std::string("")) : nullptr;
        })
        .addFunction("create_image", [](StageNode* n, TexturePtr texture) -> ui::Image* {
            auto* m = dynamic_cast<ui::UIManager*>(n);
            // "texture" is a required param.
            return m ? m->create_child<ui::Image>(texture) : nullptr;
        })
        .addFunction("create_frame", [](StageNode* n) -> ui::Frame* {
            auto* m = dynamic_cast<ui::UIManager*>(n);
            return m ? m->create_child<ui::Frame>() : nullptr;
        })
        .addFunction("create_progress_bar", [](StageNode* n) -> ui::ProgressBar* {
            auto* m = dynamic_cast<ui::UIManager*>(n);
            return m ? m->create_child<ui::ProgressBar>() : nullptr;
        })
        .endClass()
        // ----------------------------------------------------------------
        // Scene — passed as a constructor argument to stage node scripts,
        // and also the base for scenes defined in Lua via smlt.define_scene()
        // (see LuaScene, which forwards on_load/on_unload/on_activate/
        // on_deactivate to same-named Lua methods on its instance table).
        //
        //   scene.assets   -> AssetManager
        //   scene.input    -> InputManager
        //   scene:create_child(type_name, params)
        //   scene:create_mixin(type_name, params)
        //   scene:find_descendent_with_name(name)
        // ----------------------------------------------------------------
        .deriveClass<Scene, StageNode>("Scene")
        .addProperty("transform", [](Scene* s) -> Transform* { return s->get_transform(); })
        .addProperty("assets", [](Scene* s) -> AssetManager* { return s->assets.get(); })
        .addProperty("input", [](Scene* s) -> InputManager* { return s->input.get(); })
        .addProperty("compositor", [](Scene* s) -> SceneCompositor* { return s->compositor.get(); })
        .addProperty("app", [](Scene* s) -> Application* { return s->app.get(); })
        .addProperty("window", [](Scene* s) -> Window* { return s->window.get(); })
        .addFunction("create_child", [](Scene* scene, const char* name, luabridge::LuaRef params_table) -> StageNode* {
            return scene->create_child(name, lua_table_to_params(params_table));
        })
        .addFunction("create_mixin", [](Scene* scene, const std::string& name, luabridge::LuaRef params_table) -> StageNode* {
            return scene->create_mixin(name, lua_table_to_params(params_table));
        })
        .addFunction("find_descendent_with_name",
            [](Scene* scene, const std::string& name) -> StageNode* {
            return scene->find_descendent_with_name(name);
        })
        // Returns a Lua array of every descendent whose name starts with
        // `prefix`.
        .addFunction("find_nodes_by_name_prefix",
            [](Scene* scene, const std::string& prefix) -> luabridge::LuaRef {
            lua_State* L = smlt::get_app()->ensure_lua_ready()->lua_state();
            luabridge::LuaRef out = luabridge::newTable(L);
            int i = 1;
            for(auto& node: scene->each_descendent()) {
                if(node.name().rfind(prefix, 0) == 0) {
                    out[i++] = &node;
                }
            }
            return out;
        })
        // --- Activation load args (see Scene::load_arg_*) ---
        .addFunction("load_arg_count", [](Scene* s) { return (int)s->num_load_args(); })
        .addFunction("load_arg_int", [](Scene* s, int i) { return s->load_arg_int(i); })
        .addFunction("load_arg_float", [](Scene* s, int i) { return s->load_arg_float(i); })
        .addFunction("load_arg_string", [](Scene* s, int i) { return s->load_arg_string(i); })
        // --- Native (C++) stage node registration ---
        // Lets a Lua scene register a C++ node type by the name it was
        // registered under with register_native_stage_node_type().
        .addFunction("register_native_stage_node",
            [](Scene* s, const std::string& name) {
            return register_native_stage_node_type_on(s, name);
        })
        // --- Convenience camera/UI factories (return concrete types so
        // their methods are callable from Lua) ---
        .addFunction("create_perspective_camera",
            [](Scene* s, float fov, float aspect, float near,
               float far) -> Camera3D* {
            auto* cam = s->create_child<Camera3D>();
            if(cam) {
                cam->set_perspective_projection(Degrees(fov), aspect, near,
                                                far);
            }
            return cam;
        })
        .addFunction("create_orthographic_camera",
            [](Scene* s, float left, float right, float bottom,
               float top) -> Camera2D* {
            auto* cam = s->create_child<Camera2D>();
            if(cam) {
                cam->set_orthographic_projection(left, right, bottom, top);
            }
            return cam;
        })
        .addFunction("create_ui_manager",
            [](Scene* s) -> StageNode* {
            // Returned as StageNode* - see the create_label comment above
            // for why (multiple-inheritance pointer adjustment).
            auto* m = s->create_child<ui::UIManager>();
            return static_cast<StageNode*>(m);
        })
        .endClass()
        // ----------------------------------------------------------------
        // Layer — a single render pass created via
        // SceneCompositor:create_layer().
        // ----------------------------------------------------------------
        .beginClass<Layer>("Layer")
        .addFunction("name",       [](Layer* l) { return l->name(); })
        .addFunction("set_name",   [](Layer* l, const std::string& n) { l->set_name(n); })
        .addFunction("is_active",  [](Layer* l) { return l->is_active(); })
        .addFunction("activate",   [](Layer* l) { l->activate(); })
        .addFunction("deactivate", [](Layer* l) { l->deactivate(); })
        .addFunction("priority",   [](Layer* l) { return l->priority(); })
        .addFunction("destroy",    [](Layer* l) { return l->destroy(); })
        .addFunction("set_clear_flags", [](Layer* l, int flags) { l->set_clear_flags((uint32_t)flags); })
        .endClass()
        // ----------------------------------------------------------------
        // SceneCompositor — manages the scene's render layers. A layer
        // renders a stage-node subtree through a camera.
        //
        //   local layer = scene.compositor:create_layer(stage_node, camera_node, 0)
        //   local layer = scene.compositor:find_layer(name)
        //   scene.compositor:destroy_all_layers()
        // ----------------------------------------------------------------
        .beginClass<SceneCompositor>("SceneCompositor")
        .addFunction("create_layer",
            [](SceneCompositor* c, StageNode* subtree, StageNode* camera_node,
               int32_t priority) -> LayerPtr {
            auto camera = dynamic_cast<Camera*>(camera_node);
            if(!camera) {
                S_ERROR("SceneCompositor.create_layer: second argument must "
                        "be a Camera node");
                return LayerPtr();
            }
            return c->create_layer(subtree, camera, priority);
        })
        .addFunction("find_layer", [](SceneCompositor* c, const std::string& name) -> LayerPtr {
            return c->find_layer(name);
        })
        .addFunction("destroy_all_layers", [](SceneCompositor* c) { c->destroy_all_layers(); })
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
        .addFunction("width", [](Window* w) { return (int)w->width(); })
        .addFunction("height", [](Window* w) { return (int)w->height(); })
        .addFunction("show_cursor", [](Window* w, bool shown) { w->show_cursor(shown); })
        .addFunction("input_state", [](Window* w) -> InputState* { return w->input_state.get(); })
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
        .addFunction("create_texture",   [](AssetManager* am, int w, int h) { return am->create_texture((uint16_t)w, (uint16_t)h); })
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
        .addFunction("set_pixel",
            [](Texture* t, int x, int y, int r, int g, int b, int a) {
            t->set_pixel((uint16_t)x, (uint16_t)y, (uint8_t)r, (uint8_t)g,
                         (uint8_t)b, (uint8_t)a);
        })
        .addFunction("width", [](Texture* t) { return (int)t->width(); })
        .addFunction("height", [](Texture* t) { return (int)t->height(); })
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
        // InputState — raw keyboard/mouse state (window.input_state).
        //
        //   local mp = scene.window.input_state:mouse_position(0)
        //   local down = scene.window.input_state:mouse_button_state(0, 0)
        //   local left = scene.window.input_state:keyboard_key_state(0, smlt.KEY.LEFT)
        // ----------------------------------------------------------------
        .beginClass<InputState>("InputState")
        .addFunction("mouse_position",
            [](InputState* s, int mouse_id) { return s->mouse_position(MouseID(mouse_id)); })
        .addFunction("mouse_button_state",
            [](InputState* s, int mouse_id, int button) {
            return s->mouse_button_state(MouseID(mouse_id), (MouseButtonID)button);
        })
        .addFunction("keyboard_key_state",
            [](InputState* s, int keyboard_id, int code) {
            return s->keyboard_key_state(KeyboardID(keyboard_id), (KeyboardCode)code);
        })
        .addFunction("mouse_axis_state",
            [](InputState* s, int mouse_id, int axis) {
            return s->mouse_axis_state(MouseID(mouse_id), (MouseAxis)axis);
        })
        .endClass()
        // Mouse axis ids (for InputState:mouse_axis_state).
        .beginNamespace("MouseAxis")
        .addVariable("X", (int)MOUSE_AXIS_X)
        .addVariable("Y", (int)MOUSE_AXIS_Y)
        .addVariable("WHEEL", (int)MOUSE_AXIS_WHEEL)
        .addVariable("WHEEL_HORIZONTAL", (int)MOUSE_AXIS_WHEEL_HORIZONTAL)
        .endNamespace()
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
        // Color — RGBA colour (components 0..1).
        // ----------------------------------------------------------------
        .beginClass<Color>("Color")
        .addConstructor<void(*)(float, float, float, float)>()
        .addProperty("r", &Color::r, &Color::r)
        .addProperty("g", &Color::g, &Color::g)
        .addProperty("b", &Color::b, &Color::b)
        .addProperty("a", &Color::a, &Color::a)
        .addStaticFunction("black", &Color::black)
        .addStaticFunction("white", &Color::white)
        .addStaticFunction("red", &Color::red)
        .addStaticFunction("green", &Color::green)
        .addStaticFunction("blue", &Color::blue)
        .addStaticFunction("none", &Color::none)
        .endClass()
        // ----------------------------------------------------------------
        // Ray — a world-space ray with hit tests, e.g. for cursor picking.
        // Obtain one from Camera:screen_ray(); intersects_aabb returns the
        // hit distance (or nil) so callers can pick the nearest candidate.
        // ----------------------------------------------------------------
        .beginClass<Ray>("Ray")
        .addFunction("intersects_aabb",
            [](Ray* r, const Vec3& min, const Vec3& max) -> luabridge::LuaRef {
            lua_State* L = smlt::get_app()->ensure_lua_ready()->lua_state();
            AABB aabb;
            aabb.set_min_max(min, max);
            auto t = r->intersects_aabb(aabb);
            if(!t) return luabridge::LuaRef(L);
            return luabridge::LuaRef(L, (float)*t);
        })
        .addFunction("intersects_plane",
            [](Ray* r, const Vec3& point, const Vec3& normal) -> luabridge::LuaRef {
            lua_State* L = smlt::get_app()->ensure_lua_ready()->lua_state();
            Vec3 intersection;
            Plane plane(normal, normal.dot(point));
            if(!r->intersects_plane(plane, &intersection)) {
                return luabridge::LuaRef(L);
            }
            luabridge::LuaRef t = luabridge::newTable(L);
            t["x"] = intersection.x;
            t["y"] = intersection.y;
            t["z"] = intersection.z;
            return t;
        })
        .endClass()
        // ----------------------------------------------------------------
        // Camera hierarchy — for creating render layers from Lua. Concrete
        // projection setup happens in Scene:create_perspective_camera() /
        // create_orthographic_camera().
        // ----------------------------------------------------------------
        .deriveClass<Camera, StageNode>("Camera")
        .addFunction("unproject",
            [](Camera* cam, Window* win, float x, float y, float z) -> luabridge::LuaRef {
            lua_State* L = smlt::get_app()->ensure_lua_ready()->lua_state();
            Viewport viewport(VIEWPORT_TYPE_FULL);
            auto p = cam->unproject_point(*win, viewport, Vec3(x, y, z));
            if(!p) return luabridge::LuaRef(L);
            luabridge::LuaRef t = luabridge::newTable(L);
            t["x"] = p->x; t["y"] = p->y; t["z"] = p->z;
            return t;
        })
        // A world-space ray through a screen-space point (origin bottom-left).
        .addFunction("screen_ray",
            [](Camera* cam, Window* win, float x, float y) -> luabridge::LuaRef {
            lua_State* L = smlt::get_app()->ensure_lua_ready()->lua_state();
            Viewport viewport(VIEWPORT_TYPE_FULL);
            auto near = cam->unproject_point(*win, viewport, Vec3(x, y, 0.0f));
            auto far = cam->unproject_point(*win, viewport, Vec3(x, y, 1.0f));
            if(!near || !far) return luabridge::LuaRef(L);
            return luabridge::LuaRef(L, Ray(*near, (*far - *near).normalized()));
        })
        .addFunction("project",
            [](Camera* cam, Window* win, float x, float y, float z) -> luabridge::LuaRef {
            lua_State* L = smlt::get_app()->ensure_lua_ready()->lua_state();
            Viewport viewport(VIEWPORT_TYPE_FULL);
            auto p = cam->project_point(*win, viewport, Vec3(x, y, z));
            if(!p) return luabridge::LuaRef(L);
            luabridge::LuaRef t = luabridge::newTable(L);
            t["x"] = p->x; t["y"] = p->y; t["z"] = p->z;
            return t;
        })
        .endClass()
        .deriveClass<Camera2D, Camera>("Camera2D")
        .endClass()
        .deriveClass<Camera3D, Camera>("Camera3D")
        .endClass()
        // ----------------------------------------------------------------
        // UI widgets. The base Widget bindings apply to every concrete
        // widget below (Label/Button/Image/Frame/ProgressBar).
        //
        //   local label = ui:create_label()
        //   label:resize(100, 20)
        //   label:set_text("hello")
        // ----------------------------------------------------------------
        .deriveClass<ui::Widget, StageNode>("Widget")
        .addProperty("transform", [](ui::Widget* w) -> Transform* { return w->get_transform(); })
        .addFunction("resize", [](ui::Widget* w, int pw, int ph) { w->resize(Px(pw), Px(ph)); })
        .addFunction("set_anchor_point", [](ui::Widget* w, float x, float y) { w->set_anchor_point(NormalizedFloat(x), NormalizedFloat(y)); })
        .addFunction("set_text", [](ui::Widget* w, const std::string& s) { w->set_text(s); })
        .addFunction("set_text_alignment", [](ui::Widget* w, int a) { w->set_text_alignment((TextAlignment)a); })
        .addFunction("set_background_color", [](ui::Widget* w, const Color& c) { w->set_background_color(c); })
        .addFunction("set_foreground_color", [](ui::Widget* w, const Color& c) { w->set_foreground_color(c); })
        .addFunction("set_border_color", [](ui::Widget* w, const Color& c) { w->set_border_color(c); })
        .addFunction("set_text_color", [](ui::Widget* w, const Color& c) { w->set_text_color(c); })
        .addFunction("set_border_width", [](ui::Widget* w, int v) { w->set_border_width(Px(v)); })
        .addFunction("set_border_radius", [](ui::Widget* w, int v) { w->set_border_radius(Px(v)); })
        .addFunction("set_opacity", [](ui::Widget* w, float a) { w->set_opacity(NormalizedFloat(a)); })
        .addFunction("set_padding", [](ui::Widget* w, int v) { w->set_padding(Px(v)); })
        .addFunction("set_padding4", [](ui::Widget* w, int l, int r, int b, int t) { w->set_padding(Px(l), Px(r), Px(b), Px(t)); })
        .addFunction("set_background_image", [](ui::Widget* w, TexturePtr t) { w->set_background_image(t); })
        .addFunction("set_foreground_image", [](ui::Widget* w, TexturePtr t) { w->set_foreground_image(t); })
        .addFunction("is_pressed", [](ui::Widget* w) { return w->is_pressed(); })
        .addFunction("click", [](ui::Widget* w) { w->click(); })
        .addFunction("on_clicked", [](ui::Widget* w, luabridge::LuaRef fn) {
            auto cb = make_lua_callback(fn);
            if(cb) {
                w->signal_clicked().connect([cb]() { (*cb)(); });
            }
        })
        .endClass()
        .deriveClass<ui::Label, ui::Widget>("Label")
        .endClass()
        .deriveClass<ui::Button, ui::Widget>("Button")
        .endClass()
        .deriveClass<ui::Image, ui::Widget>("Image")
        .addFunction("set_texture", [](ui::Image* i, TexturePtr t) { i->set_texture(t); })
        .addFunction("set_source_rect",
            [](ui::Image* i, int x, int y, int w, int h) {
            i->set_source_rect(UICoord(Px(x), Px(y)), UICoord(Px(w), Px(h)));
        })
        .endClass()
        .deriveClass<ui::ProgressBar, ui::Widget>("ProgressBar")
        .addFunction("set_value", [](ui::ProgressBar* p, float v) { p->set_value(v); })
        .addFunction("set_range", [](ui::ProgressBar* p, float mn, float mx) { p->set_range(mn, mx); })
        .addFunction("set_fraction", [](ui::ProgressBar* p, float f) { p->set_fraction(NormalizedFloat(f)); })
        .endClass()
        .deriveClass<ui::Frame, ui::Widget>("Frame")
        .addFunction("pack_child", [](ui::Frame* f, ui::Widget* c) { return f->pack_child(c); })
        .addFunction("set_layout_direction", [](ui::Frame* f, int d) { f->set_layout_direction((LayoutDirection)d); })
        .addFunction("set_space_between", [](ui::Frame* f, int v) { f->set_space_between(Px(v)); })
        .endClass()
        // ----------------------------------------------------------------
        // UIManager — registered for completeness; widget factories are on
        // StageNode (see the comment there) because of the multiple-
        // inheritance pointer issue.
        // ----------------------------------------------------------------
        .deriveClass<ui::UIManager, StageNode>("UIManager")
        .endClass()
        // ----------------------------------------------------------------
        // Keyboard codes (smlt.KEY.*) and text alignment / layout enums.
        // ----------------------------------------------------------------
        .beginNamespace("KEY")
        .addVariable("A", (int)KEYBOARD_CODE_A)
        .addVariable("B", (int)KEYBOARD_CODE_B)
        .addVariable("C", (int)KEYBOARD_CODE_C)
        .addVariable("D", (int)KEYBOARD_CODE_D)
        .addVariable("E", (int)KEYBOARD_CODE_E)
        .addVariable("F", (int)KEYBOARD_CODE_F)
        .addVariable("G", (int)KEYBOARD_CODE_G)
        .addVariable("H", (int)KEYBOARD_CODE_H)
        .addVariable("I", (int)KEYBOARD_CODE_I)
        .addVariable("J", (int)KEYBOARD_CODE_J)
        .addVariable("K", (int)KEYBOARD_CODE_K)
        .addVariable("L", (int)KEYBOARD_CODE_L)
        .addVariable("M", (int)KEYBOARD_CODE_M)
        .addVariable("N", (int)KEYBOARD_CODE_N)
        .addVariable("O", (int)KEYBOARD_CODE_O)
        .addVariable("P", (int)KEYBOARD_CODE_P)
        .addVariable("Q", (int)KEYBOARD_CODE_Q)
        .addVariable("R", (int)KEYBOARD_CODE_R)
        .addVariable("S", (int)KEYBOARD_CODE_S)
        .addVariable("T", (int)KEYBOARD_CODE_T)
        .addVariable("U", (int)KEYBOARD_CODE_U)
        .addVariable("V", (int)KEYBOARD_CODE_V)
        .addVariable("W", (int)KEYBOARD_CODE_W)
        .addVariable("X", (int)KEYBOARD_CODE_X)
        .addVariable("Y", (int)KEYBOARD_CODE_Y)
        .addVariable("Z", (int)KEYBOARD_CODE_Z)
        .addVariable("SPACE", (int)KEYBOARD_CODE_SPACE)
        .addVariable("RETURN", (int)KEYBOARD_CODE_RETURN)
        .addVariable("ESCAPE", (int)KEYBOARD_CODE_ESCAPE)
        .addVariable("TAB", (int)KEYBOARD_CODE_TAB)
        .addVariable("LEFT", (int)KEYBOARD_CODE_LEFT)
        .addVariable("RIGHT", (int)KEYBOARD_CODE_RIGHT)
        .addVariable("UP", (int)KEYBOARD_CODE_UP)
        .addVariable("DOWN", (int)KEYBOARD_CODE_DOWN)
        .addVariable("LSHIFT", (int)KEYBOARD_CODE_LSHIFT)
        .addVariable("RSHIFT", (int)KEYBOARD_CODE_RSHIFT)
        .addVariable("PLUS", (int)KEYBOARD_CODE_EQUALS)
        .addVariable("MINUS", (int)KEYBOARD_CODE_MINUS)
        .addVariable("F1", (int)KEYBOARD_CODE_F1)
        .endNamespace()
        .beginNamespace("TextAlignment")
        .addVariable("LEFT", (int)ui::TEXT_ALIGNMENT_LEFT)
        .addVariable("CENTER", (int)ui::TEXT_ALIGNMENT_CENTER)
        .addVariable("RIGHT", (int)ui::TEXT_ALIGNMENT_RIGHT)
        .endNamespace()
        .beginNamespace("LayoutDirection")
        .addVariable("TOP_TO_BOTTOM", (int)ui::LAYOUT_DIRECTION_TOP_TO_BOTTOM)
        .addVariable("LEFT_TO_RIGHT", (int)ui::LAYOUT_DIRECTION_LEFT_TO_RIGHT)
        .endNamespace()
        // ----------------------------------------------------------------
        // Utility functions
        // ----------------------------------------------------------------
        .addFunction("stage_node_meta", &stage_node_meta)
        // Environment access (read-only), so scripts can adapt to
        // headless/test runs.
        .addFunction("getenv", [](const std::string& name) -> std::string {
            const char* v = std::getenv(name.c_str());
            return v ? std::string(v) : std::string();
        })
        // Testing aid: dispatch a synthetic left-click at screen-space
        // (bottom-left origin) coordinates, as if the platform had sent it.
        .addFunction("simulate_mouse_click", [](Window* w, float x, float y) {
            w->on_mouse_down(MouseID(0), 0, (int32_t)x, (int32_t)y, false);
            w->on_mouse_up(MouseID(0), 0, (int32_t)x, (int32_t)y, false);
        })
        // ----------------------------------------------------------------
        // LuaStageNode — exposed to Lua as smlt.LuaNode, derived from the
        // generic smlt.StageNode registered above (so Lua-authored nodes
        // also get destroy()/is_destroyed()/transform for free). Nothing
        // constructs this from Lua directly by name — see below.
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
        .deriveClass<LuaStageNode, StageNode>("LuaNode")
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
        .addFunction("has_lua_method", &LuaStageNode::has_lua_method)
        .endClass();

    // Engine/game extensions (custom C++ node types, etc). Run last so the
    // built-in smlt.* namespace/classes above already exist.
    for(auto& hook: lua_binding_hooks()) {
        hook(state);
    }
}
} // namespace smlt
