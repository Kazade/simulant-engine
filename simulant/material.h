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
#include "generic/managed.h"
#include "types.h"
#include "loadable.h"
#include "interfaces/updateable.h"

namespace smlt {

const std::string DIFFUSE_PROPERTY = "s_material_diffuse";
const std::string AMBIENT_PROPERTY = "s_material_ambient";
const std::string SPECULAR_PROPERTY = "s_material_specular";
const std::string EMISSION_PROPERTY = "s_material_emission";
const std::string SHININESS_PROPERTY = "s_material_shininess";
const std::string DIFFUSE_MAP_PROPERTY = "s_diffuse_map";
const std::string LIGHT_MAP_PROPERTY = "s_light_map";
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
const std::string TEXTURING_ENABLED_PROPERTY = "s_texturing_enabled";
const std::string POINT_SIZE_PROPERTY = "s_point_size";
const std::string POLYGON_MODE_PROPERTY = "s_polygon_mode";
const std::string COLOUR_MATERIAL_PROPERTY = "s_colour_material";
const std::string LIGHT_POSITION_PROPERTY = "s_light_position";
const std::string LIGHT_AMBIENT_PROPERTY = "s_light_ambient";
const std::string LIGHT_DIFFUSE_PROPERTY = "s_light_diffuse";
const std::string LIGHT_SPECULAR_PROPERTY = "s_light_specular";
const std::string LIGHT_CONSTANT_ATTENUATION_PROPERTY = "s_constant_attenuation";
const std::string LIGHT_LINEAR_ATTENUATION_PROPERTY = "s_linear_attenuation";
const std::string LIGHT_QUADRATIC_ATTENUATION_PROPERTY = "s_quadratic_attenuation";

const std::string VIEW_MATRIX_PROPERTY = "s_view";
const std::string MODELVIEW_PROJECTION_MATRIX_PROPERTY = "s_modelview_projection";
const std::string PROJECTION_MATRIX_PROPERTY = "s_projection";
const std::string MODELVIEW_MATRIX_PROPERTY = "s_modelview";

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

    void scroll_x(float amount);
    void scroll_y(float amount);
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

    std::string shader_variable() const;

    std::string name() const;

    MaterialPropertyType type() const;
private:
    friend class Material;

    PropertyValue(_material_impl::DefinedProperty* defined_property):
        defined_property_(defined_property) {}

    _material_impl::DefinedProperty* defined_property_;
    smlt::optional<smlt::any> value_;
};

namespace _material_impl {
    struct DefinedProperty {
        uint32_t order;  // Keep track of the order properties were defined
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

        void set_light_map(TextureID texture_id) {
            TextureUnit unit;
            unit.texture_id = texture_id;

            set_property_value(LIGHT_MAP_PROPERTY, unit);
        }

        TextureUnit* diffuse_map() const {
            return nullptr;
        }

        TextureUnit* light_map() const {
            return nullptr; // FIXME: How to return a pointer to an optional any...
        }

        TextureUnit* normal_map() const {
            return nullptr; // FIXME: How to return a pointer to an optional any...
        }

        TextureUnit* specular_map() const {
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

        bool is_blending_enabled() const {
            return property(BLENDING_ENABLE_PROPERTY)->value<bool>();
        }

        void set_blending_enabled(bool v) {
            set_property_value(BLENDING_ENABLE_PROPERTY, v);
        }

        void set_blend_func(BlendType b) {
            set_property_value(BLEND_FUNC_PROPERTY, (int) b);
        }

        BlendType blend_func() const {
            return (BlendType) property(BLEND_FUNC_PROPERTY)->value<int>();
        }

        bool is_blended() const {
            return blend_func() != BLEND_NONE;
        }

        void set_depth_write_enabled(bool v) {
            set_property_value(DEPTH_WRITE_ENABLED_PROPERTY, v);
        }

        bool is_depth_write_enabled() const {
            return property(DEPTH_WRITE_ENABLED_PROPERTY)->value<bool>();
        }

        void set_cull_mode(CullMode mode) {
            set_property_value(CULL_MODE_PROPERTY, (int) mode);
        }

        CullMode cull_mode() const {
            return (CullMode) property(CULL_MODE_PROPERTY)->value<int>();
        }

        void set_depth_test_enabled(bool v) {
            set_property_value(DEPTH_TEST_ENABLED_PROPERTY, v);
        }

        bool is_depth_test_enabled() const {
            return property(DEPTH_TEST_ENABLED_PROPERTY)->value<bool>();
        }

        void set_lighting_enabled(bool v) {
            set_property_value(LIGHTING_ENABLED_PROPERTY, v);
        }

        bool is_lighting_enabled() const {
            return property(LIGHTING_ENABLED_PROPERTY)->value<bool>();
        }

        void set_texturing_enabled(bool v) {
            set_property_value(TEXTURING_ENABLED_PROPERTY, v);
        }

        bool is_texturing_enabled() const {
            return property(TEXTURING_ENABLED_PROPERTY)->value<bool>();
        }

        float point_size() const {
            return property(POINT_SIZE_PROPERTY)->value<float>();
        }

        PolygonMode polygon_mode() const {
            return (PolygonMode) property(POLYGON_MODE_PROPERTY)->value<int>();
        }

        ShadeModel shade_model() const {
            return (ShadeModel) property(SHADE_MODEL_PROPERTY)->value<int>();
        }

        ColourMaterial colour_material() const {
            return (ColourMaterial) property(COLOUR_MATERIAL_PROPERTY)->value<int>();
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
    MaterialPass(Material* material):
        material_(material) {

    }

    void set_iteration_type(IterationType iteration) {
        iteration_type_ = iteration;
    }

    IterationType iteration_type() const {
        return iteration_type_;
    }

    GPUProgramID gpu_program_id() const;

    void set_gpu_program(GPUProgramID program) {
        // If the renderer doesn't support GPU programs then this
        // will be an empty ID
        if(program) {
            program_ = program.fetch();
        }
    }

    uint8_t max_iterations() const {
        return max_iterations_;
    }

    const Material* material() const {
        return material_;
    }

private:
    Material* material_;
    ShaderUnit shader_;

    IterationType iteration_type_ = ITERATION_TYPE_ONCE;
    uint8_t max_iterations_ = 1;

    std::unordered_map<std::string, PropertyValue> property_values_;

    GPUProgramPtr program_;
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

    static const std::unordered_map<std::string, std::string> BUILT_IN_NAMES;

    Material(MaterialID id, AssetManager *asset_manager);

    template<typename T>
    void define_property(
        MaterialPropertyType type,
        std::string name,
        std::string shader_variable,
        const T& default_value
    ) {
        _material_impl::DefinedProperty prop;
        prop.order = defined_properties_.size();
        prop.name = name;
        prop.type = type;
        prop.default_value = smlt::any(default_value);
        prop.shader_variable = shader_variable;

        defined_properties_.insert(std::make_pair(name, prop));
    }

    void define_property(
        MaterialPropertyType type,
        std::string name,
        std::string shader_variable
    ) {
        _material_impl::DefinedProperty prop;
        prop.order = defined_properties_.size();
        prop.name = name;
        prop.type = type;
        prop.shader_variable = shader_variable;

        defined_properties_.insert(std::make_pair(name, prop));
    }

    MaterialPropertyType defined_property_type(const std::string& name) const {
        return defined_properties_.at(name).type;
    }

    std::vector<std::string> defined_properties_by_type(MaterialPropertyType type) const;
    std::vector<std::string> defined_custom_properties() const;

    bool property_is_defined(const std::string& name) const {
        return defined_properties_.count(name) > 0;
    }

// ---------- Passes ------------------------
    void set_pass_count(uint8_t pass_count) {
        if(pass_count < pass_count_) {
            // We're removing a pass, we should reset any that are now unused
            for(auto i = pass_count; i < pass_count_; ++i) {
                passes_[i] = MaterialPass(this);
            }
        }

        pass_count_ = pass_count;
    }

    uint8_t pass_count() const {
        return pass_count_;
    }

    MaterialPass* pass(uint8_t pass) {
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


    /* Assignment operator and copy constructor must be private
     * to prevent accidental copying. However the object manager needs
     * to be able to clone materials, hence the friendship.
     */

protected:
    friend class _object_manager_impl::ObjectManagerBase<
        MaterialID, Material, std::shared_ptr<smlt::Material>,
        _object_manager_impl::ToSharedPtr<smlt::Material>
    >;

    Material(const Material& rhs);
    Material& operator=(const Material& rhs);

    MaterialPtr new_clone() {
        return std::shared_ptr<Material>(new Material(*this));
    }
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
