#pragma once

#include "../generic/optional.h"
#include "../utils/unicode.h"

#include "../assets/texture_flags.h"
#include "../nodes/geoms/geom_culler_opts.h"
#include "../nodes/ui/ui_config.h"

#include "limited_string.h"
#include <initializer_list>
#include <map>
#include <memory>
#include <variant>
#include <vector>

namespace smlt {

class StageNode;

namespace ui {
struct WidgetStyle;
typedef std::shared_ptr<WidgetStyle> WidgetStylePtr;
typedef std::weak_ptr<WidgetStyle> WidgetStyleRef;
} // namespace ui

typedef LimitedString<32> ParamKey;
typedef std::vector<ParamKey> ParamKeys;

typedef std::variant<float, FloatArray, int, IntArray, bool, BoolArray,
                     std::string, TextureRef, MeshRef, ParticleScriptRef,
                     PrefabRef, ui::UIConfig, ui::WidgetStyleRef,
                     GeomCullerOptions, TextureFlags, StageNode*>
    ParamValue;

/*
   Generic Key <> Value type for passing arguments to things like stage nodes.
   The available types are modelled on Blender's custom properties with the
   exception that bool and bool array can be represented instead by int an int
   array. Additional types are required for stage node parameters.

   Vector and Matrix types are implicitly converted to FloatArray.
*/

class Params {
public:
    Params() = default;
    Params(const std::initializer_list<std::pair<const char*, ParamValue>>&
               params) {
        for(auto& param: params) {
            set_arg(param.first, param.second);
        }
    }

    bool contains(const char* name) const {
        return dict_.count(name);
    }

    template<typename T>
    optional<T> get(const char* name) const {
        auto it = dict_.find(name);
        if(it == dict_.end()) {
            return no_value;
        }

        try {
            return std::get<T>(it->second);
        } catch(std::bad_variant_access&) {
            return no_value;
        }
    }

    ParamKeys arg_names() const {
        ParamKeys ret;
        for(auto& p: dict_) {
            ret.push_back(p.first);
        }
        return ret;
    }

    template<typename T>
    Params set(const char* name, T value) {
        set_arg(name, value);
        return *this;
    }

    optional<ParamValue> raw(const char* name) const {
        auto it = dict_.find(name);
        if(it != dict_.end()) {
            return it->second;
        }

        return no_value;
    }

private:
    std::map<ParamKey, ParamValue> dict_;

    template<typename T>
    bool set_arg(const char* name, T value) {
        auto existed = dict_.count(name);
        dict_[name] = value;
        return !existed;
    }

    bool set_arg(const char* name, const ui::Px& px) {
        return set_arg(name, px.value);
    }

    bool set_arg(const char* name, const Vec2& vec) {
        return set_arg(name, FloatArray({vec.x, vec.y}));
    }

    bool set_arg(const char* name, const Vec3& vec) {
        return set_arg(name, FloatArray({vec.x, vec.y, vec.z}));
    }

    bool set_arg(const char* name, const Vec4& vec) {
        return set_arg(name, FloatArray({vec.x, vec.y, vec.z, vec.w}));
    }

    bool set_arg(const char* name, const Quaternion& vec) {
        return set_arg(name, FloatArray({vec.x, vec.y, vec.z, vec.w}));
    }

    bool set_arg(const char* name, const char* text) {
        return set_arg(name, std::string(text));
    }

    bool set_arg(const char* name, const unicode& text) {
        return set_arg(name, text.encode());
    }

    bool set_arg(const char* name, ParamValue value) {
        auto existed = dict_.count(name);
        dict_.insert(std::make_pair(name, value));
        return !existed;
    }
};

} // namespace smlt
