/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MATERIAL_H
#define MATERIAL_H


#include <unordered_map>

#include "resource.h"
#include "generic/identifiable.h"
#include "generic/optional.h"
#include "generic/any/any.h"
#include "types.h"

namespace smlt {

const std::string DIFFUSE_PROPERTY = "s_diffuse";
const std::string AMBIENT_PROPERTY = "s_ambient";
const std::string SPECULAR_PROPERTY = "s_specular";
const std::string EMISSION_PROPERTY = "s_emission";
const std::string SHININESS_PROPERTY = "s_shininess";
const std::string DIFFUSE_MAP_PROPERTY = "s_diffuse_map";
const std::string NORMAL_MAP_PROPERTY = "s_normal_map";
const std::string SPECULAR_MAP_PROPERTY = "s_specular_map";
const std::string DEPTH_WRITE_ENABLED_PROPERTY = "s_depth_write";
const std::string DEPTH_TEST_ENABLED_PROPERTY = "s_depth_test";
const std::string DEPTH_FUNC_PROPERTY = "s_depth_func";
const std::string BLENDING_ENABLE_PROPERTY = "s_blending_enabled";
const std::string BLEND_FUNC_PROPERTY = "s_blend_func";
const std::string CULLING_ENABLED_PROPERTY = "s_culling_enabled";
const std::string CULL_MODE_PROPERTY = "s_cull_mode";
const std::string SHADE_MODEL_PROPERTY = "s_shade_model";
const std::string LIGHTING_ENABLED_PROPERTY = "s_lighting_enabled";


enum MaterialPropertyType {
    MATERIAL_PROPERTY_TYPE_TEXTURE,
    MATERIAL_PROPERTY_TYPE_BOOL,
    MATERIAL_PROPERTY_TYPE_INT,
    MATERIAL_PROPERTY_TYPE_FLOAT,
    MATERIAL_PROPERTY_TYPE_VEC2,
    MATERIAL_PROPERTY_TYPE_VEC3,
    MATERIAL_PROPERTY_TYPE_VEC4
};

enum IterationType {
    ITERATION_TYPE_ONCE,
    ITERATION_TYPE_N,
    ITERATION_TYPE_ONCE_PER_LIGHT
};

/* Value type, if the type is texture */
struct TextureUnit {
    std::string filename;
    TextureID texture_id;
    Mat4 texture_matrix;
};

namespace _material_impl {
    class DefinedProperty;
}

class PropertyValue {
public:
    /* Has this property had the default overridden? */
    bool is_set() const {
        return value_.has_value();
    }

    template<typename T>
    T value() const;

private:
    friend class Material;

    PropertyValue(_material_impl::DefinedProperty* defined_property):
        defined_property_(defined_property) {}

    _material_impl::DefinedProperty* defined_property_;
    smlt::optional<smlt::any> value_;
};

namespace _material_impl {
    struct DefinedProperty {
        std::string name;
        std::string shader_variable;
        MaterialPropertyType type;
        smlt::optional<smlt::any> default_value;
    };

    class PropertyValueHolder {
    public:
        void set_property_value(const std::string& name, const std::string& value);
        void set_property_value(const std::string& name, bool value);
        void set_property_value(const std::string& name, Vec4 value);
        void set_property_value(const std::string& name, TextureUnit unit);

        const PropertyValue* property(const std::string& name) const {
            auto it = property_values_.find(name);
            if(it == property_values_.end()) {
                return nullptr;
            }

            return &it->second;
        }

        void unset_property_value(const std::string& name);

        // ---------- Helpers -----------------------

        void set_emission(const Colour& colour) {
            set_property_value(EMISSION_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
        }

        void set_specular(const Colour& colour) {
            set_property_value(SPECULAR_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
        }

        void set_ambient(const Colour& colour) {
            set_property_value(AMBIENT_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
        }

        void set_diffuse(const Colour& colour) {
            set_property_value(DIFFUSE_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
        }

        void set_shininess(float shininess) {
            set_property_value(SHININESS_PROPERTY, shininess);
        }

        void set_diffuse_map(TextureID texture_id) {
            TextureUnit unit;
            unit.texture_id = texture_id;

            set_property_value(DIFFUSE_MAP_PROPERTY, unit);
        }

        TextureUnit* diffuse_map() const {
            return nullptr; // FIXME: How to return a pointer to an optional any...
        }

        Colour emission() const {
            auto v = property(EMISSION_PROPERTY)->value<Vec4>();
            return Colour(v.x, v.y, v.z, v.w);
        }

        Colour specular() const {
            auto v = property(SPECULAR_PROPERTY)->value<Vec4>();
            return Colour(v.x, v.y, v.z, v.w);
        }

        Colour ambient() const {
            auto v = property(AMBIENT_PROPERTY)->value<Vec4>();
            return Colour(v.x, v.y, v.z, v.w);
        }

        Colour diffuse() const {
            auto v = property(DIFFUSE_PROPERTY)->value<Vec4>();
            return Colour(v.x, v.y, v.z, v.w);
        }

        float shininess() const {
            return property(SHININESS_PROPERTY)->value<float>();
        }

        bool blending_enabled() const {
            return property(BLENDING_ENABLE_PROPERTY)->value<bool>();
        }

        void set_blending_enabled(bool v) {
            set_property_value(BLENDING_ENABLE_PROPERTY, v);
        }

        void set_depth_write_enabled(bool v) {
            set_property_value(DEPTH_WRITE_ENABLED_PROPERTY, v);
        }

    protected:
        std::unordered_map<std::string, PropertyValue> property_values_;
    };
}

class ShaderUnit {
public:
    bool is_compiled() const;
    GPUProgramID shader_id() const;
    std::string filename() const;
};

class MaterialPass:
    public _material_impl::PropertyValueHolder  {
public:

private:
    ShaderUnit shader_;

    IterationType iteration_type_ = ITERATION_TYPE_ONCE;
    uint8_t max_iterations_ = 1;

    std::unordered_map<std::string, PropertyValue> property_values_;
};


class Material:
    public Resource,
    public Loadable,
    public generic::Identifiable<MaterialID>,
    public Managed<Material>,
    public Updateable,
    public _material_impl::PropertyValueHolder {

public:
    struct BuiltIns {
        static const std::string DEFAULT;
        static const std::string TEXTURE_ONLY;
        static const std::string DIFFUSE_ONLY;
        static const std::string ALPHA_TEXTURE;
        static const std::string DIFFUSE_WITH_LIGHTING;
        static const std::string MULTITEXTURE2_MODULATE;
        static const std::string MULTITEXTURE2_ADD;
        static const std::string TEXTURE_WITH_LIGHTMAP;
        static const std::string TEXTURE_WITH_LIGHTMAP_AND_LIGHTING;
        static const std::string MULTITEXTURE2_MODULATE_WITH_LIGHTING;
        static const std::string SKYBOX;
        static const std::string TEXTURED_PARTICLE;
        static const std::string DIFFUSE_PARTICLE;
    };

    static const std::map<std::string, std::string> BUILT_IN_NAMES;

    Material(MaterialID id, AssetManager *asset_manager);

    template<typename T>
    void define_property(
        MaterialPropertyType type,
        std::string name,
        std::string shader_variable,
        const smlt::optional<T>& default_value=smlt::optional<T>()
    ) {
        _material_impl::DefinedProperty prop;
        prop.name = name;
        prop.type = type;
        //FIXME: set default
        prop.shader_variable = shader_variable;

        defined_properties_.insert(std::make_pair(name, prop));
    }


// ---------- Passes ------------------------
    void set_pass_count(uint8_t pass_count);

    const MaterialPass* pass(uint8_t pass) const {
        if(pass < pass_count_) {
            return &passes_[pass];
        }

        return nullptr;
    }

    void each(std::function<void (uint32_t, MaterialPass*)> callback) {
        for(auto i = 0; i != pass_count_; ++i) {
            callback(i, &passes_[i]);
        }
    }

    void update(float dt);

private:
    void initialize_default_properties();

    std::unordered_map<std::string, _material_impl::DefinedProperty> defined_properties_;

    uint8_t pass_count_ = 0;
    std::array<MaterialPass, 4> passes_;
};


template<typename T>
T PropertyValue::value() const {
    T ret;

    if(!is_set()) {
        if(!defined_property_->default_value.has_value()) {
            L_WARN("Accessed null property value");
            return T();
        } else {
            ret = smlt::any_cast<T>(defined_property_->default_value.value());
        }
    } else {
        ret = smlt::any_cast<T>(value_.value());
    }

    return ret;
}

}

#endif // MATERIAL_H
