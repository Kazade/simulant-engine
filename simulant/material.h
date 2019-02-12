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
#include "generic/managed.h"
#include "types.h"
#include "loadable.h"
#include "interfaces/updateable.h"


class MaterialTest;

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
const std::string DEPTH_WRITE_ENABLED_PROPERTY = "s_depth_write_enabled";
const std::string DEPTH_TEST_ENABLED_PROPERTY = "s_depth_test_enabled";
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
const std::string INVERSE_TRANSPOSE_MODELVIEW_MATRIX_PROPERTY = "s_inverse_transpose_modelview";

const std::string DIFFUSE_MAP_MATRIX_PROPERTY = "s_diffuse_map_matrix";
const std::string LIGHT_MAP_MATRIX_PROPERTY = "s_light_map_matrix";
const std::string NORMAL_MAP_MATRIX_PROPERTY = "s_normal_map_matrix";
const std::string SPECULAR_MAP_MATRIX_PROPERTY = "s_specular_map_matrix";

enum MaterialPropertyType {
    MATERIAL_PROPERTY_TYPE_TEXTURE,
    MATERIAL_PROPERTY_TYPE_BOOL,
    MATERIAL_PROPERTY_TYPE_INT,
    MATERIAL_PROPERTY_TYPE_FLOAT,
    MATERIAL_PROPERTY_TYPE_VEC2,
    MATERIAL_PROPERTY_TYPE_VEC3,
    MATERIAL_PROPERTY_TYPE_VEC4,
    MATERIAL_PROPERTY_TYPE_MAT3,
    MATERIAL_PROPERTY_TYPE_MAT4
};

enum IterationType {
    ITERATION_TYPE_ONCE,
    ITERATION_TYPE_N,
    ITERATION_TYPE_ONCE_PER_LIGHT
};

namespace _material_impl {
    class PropertyValueHolder;

    const static int MAX_PASSES = 4;
    const static int MAX_DEFINED_PROPERTIES = 64;
}

/* Value type, if the type is texture */
struct TextureUnit {
    friend class ::MaterialTest;

    friend class _material_impl::PropertyValueHolder;

    TextureID texture_id;

    Mat4& texture_matrix() {
        return *texture_matrix_;
    }

    const Mat4& texture_matrix() const {
        return *texture_matrix_;
    }

    void scroll_x(float amount);
    void scroll_y(float amount);

private:
    /* Shared pointer so that copying a TextureUnit also copies the matrix */
    std::shared_ptr<Mat4> texture_matrix_ = std::make_shared<Mat4>();

    /* Set when assigned as a material property to maintain a refcount */
    TexturePtr texture_;
};

namespace _material_impl {
    class DefinedProperty;
    class PropertyValueHolder;
}

class PropertyValue {
public:
    friend class _material_impl::PropertyValueHolder;

    template<typename T>
    T value() const;

    std::string shader_variable() const;
    std::string name() const;
    bool is_custom() const;

    MaterialPropertyType type() const;
private:
    PropertyValue(_material_impl::DefinedProperty* defined_property, uint8_t slot):
        defined_property_(defined_property),
        slot_(slot) {

        assert(defined_property);
    }

    _material_impl::DefinedProperty* defined_property_;
    uint8_t slot_;
};

namespace _material_impl {
    template<int>
    struct TypeForMaterialType {};

    template<>
    struct TypeForMaterialType<MATERIAL_PROPERTY_TYPE_BOOL> {
        typedef bool type;
    };

    template<>
    struct TypeForMaterialType<MATERIAL_PROPERTY_TYPE_TEXTURE> {
        typedef TextureUnit type;
    };

    template<>
    struct TypeForMaterialType<MATERIAL_PROPERTY_TYPE_INT> {
        typedef int type;
    };

    template<>
    struct TypeForMaterialType<MATERIAL_PROPERTY_TYPE_FLOAT> {
        typedef float type;
    };

    template<>
    struct TypeForMaterialType<MATERIAL_PROPERTY_TYPE_MAT3> {
        typedef Mat3 type;
    };

    template<>
    struct TypeForMaterialType<MATERIAL_PROPERTY_TYPE_MAT4> {
        typedef Mat4 type;
    };

    template<typename T>
    struct MaterialTypeForType {};

    template<>
    struct MaterialTypeForType<bool> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_BOOL;
    };

    template<>
    struct MaterialTypeForType<float> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_FLOAT;
    };

    template<>
    struct MaterialTypeForType<int> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_INT;
    };

    template<>
    struct MaterialTypeForType<Vec2> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC2;
    };

    template<>
    struct MaterialTypeForType<Vec3> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC3;
    };

    template<>
    struct MaterialTypeForType<Vec4> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC4;
    };

    template<>
    struct MaterialTypeForType<TextureUnit> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_TEXTURE;
    };

    template<>
    struct MaterialTypeForType<Mat3> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_MAT3;
    };

    template<>
    struct MaterialTypeForType<Mat4> {
        static const MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_MAT4;
    };

    template<typename T>
    constexpr const T &T_max(const T &a, const T &b) {
        return a > b ? a : b;
    }

    template<typename T, typename... Ts>
    struct max_sizeof {
        static const size_t value = T_max(sizeof(T), max_sizeof<Ts...>::value);
    };

    template<typename T>
    struct max_sizeof<T> {
        static const size_t value = sizeof(T);
    };

    /* Dirty hacky trash-all-the-things variant because std:variant and std::any are too slow */
    template<typename... Args>
    struct FastVariant {
        typedef FastVariant<Args...> this_type;

        /* Allocate data to hold the largest type */
        uint8_t data[max_sizeof<Args...>::value];

        std::function<void (this_type*)> destroy;
        std::function<void (this_type*, const this_type*)> copy;

        FastVariant() {
            set<bool>(false);
        }

        FastVariant(const this_type& rhs) {
            if(rhs.copy) {
                rhs.copy(this, &rhs);
            }
        }

        this_type& operator=(const this_type& rhs) {
            if(rhs.copy) {
                rhs.copy(this, &rhs);
            }

            return *this;
        }

        ~FastVariant() {
            if(destroy) {
                destroy(this);
            }
        }

        template<typename T>
        const T& get() const {
            return *((T*) data);
        }

        template<typename T>
        void set(const T& val) {
            if(destroy) {
                destroy(this);
            }

            new (data) T(val);

            destroy = [](this_type* _this) {
                T* thing = (T*) _this->data;
                thing->~T();
            };

            copy = [](this_type* _this, const this_type* rhs) {
                _this->set(rhs->get<T>());
            };
        }
    };

    typedef FastVariant<bool, int, float, Vec2, Vec3, Vec4, Mat3, Mat4, TextureUnit> MaterialVariant;

    struct DefinedProperty {
        uint32_t index;  // Keep track of the order properties were defined
        std::string name;
        std::string shader_variable;
        MaterialPropertyType type;
        MaterialVariant default_value;
        bool is_custom = true;

        /* Material value + pass count */
        bool slot_used[_material_impl::MAX_PASSES + 1] = {0, 0, 0, 0, 0};
        MaterialVariant values[_material_impl::MAX_PASSES + 1];
    };

    class PropertyValueHolder {
    public:
        friend class ::smlt::Material;

        PropertyValueHolder(Material* top_level, uint8_t slot):
            top_level_(top_level),
            slot_(slot) {

            if(((PropertyValueHolder*) top_level) == this) {
                assert(slot == 0);
            }

            assert(slot < _material_impl::MAX_PASSES + 1);
        }

        virtual ~PropertyValueHolder() {}

        const Material* top_level() const {
            return top_level_;
        }

        template<typename T>
        void set_property_value(const std::string& name, const T& value);

        template<typename T>
        void set_property_value(uint32_t index, const T& value);

        void set_property_value(uint32_t index, TextureID tex_id) {
            TextureUnit unit;
            unit.texture_id = tex_id;

            if(tex_id) {
                unit.texture_ = tex_id.fetch();
            }

            set_property_value(index, unit);
        }

        /* Special case to make setting texture values more straightforward */
        void set_property_value(const std::string& name, TextureID tex_id);

        PropertyValue property(const std::string& name) const;
        PropertyValue property(uint32_t defined_property_index) const;

        void unset_property_value(const std::string& name);

        // ---------- Helpers -----------------------

        void set_emission(const Colour& colour);
        void set_specular(const Colour& colour);
        void set_ambient(const Colour& colour);
        void set_diffuse(const Colour& colour);
        void set_shininess(float shininess);
        void set_diffuse_map(TextureID texture_id);
        void set_light_map(TextureID texture_id);
        TextureUnit diffuse_map() const;
        TextureUnit light_map() const;
        TextureUnit normal_map() const;
        TextureUnit specular_map() const;
        Colour emission() const;
        Colour specular() const;
        Colour ambient() const;
        Colour diffuse() const;
        float shininess() const;
        bool is_blending_enabled() const;
        void set_blending_enabled(bool v);
        void set_blend_func(BlendType b);
        BlendType blend_func() const;
        bool is_blended() const;
        void set_depth_write_enabled(bool v);
        bool is_depth_write_enabled() const;
        void set_cull_mode(CullMode mode);
        CullMode cull_mode() const;
        void set_depth_test_enabled(bool v);
        bool is_depth_test_enabled() const;
        void set_lighting_enabled(bool v);
        bool is_lighting_enabled() const;
        void set_texturing_enabled(bool v);
        bool is_texturing_enabled() const;
        float point_size() const;
        PolygonMode polygon_mode() const;
        void set_shade_model(ShadeModel model);
        ShadeModel shade_model() const;
        ColourMaterial colour_material() const;

    protected:
        /* We want to force users to use the TextureID version, hence the explicit protected override */
        void set_property_value(const std::string& name, TextureUnit unit) {
            set_property_value<TextureUnit>(name, unit);
        }

    private:
        Material* top_level_;
        uint8_t slot_;
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
    friend class Material;

    MaterialPass(Material* material, uint8_t index);

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

    GPUProgramPtr program_;
};

typedef uint8_t PropertyIndex;

class Material:
    public Resource,
    public Loadable,
    public generic::Identifiable<MaterialID>,
    public Managed<Material>,
    public Updateable,
    public _material_impl::PropertyValueHolder {

public:
    friend class GenericRenderer;
    friend class _material_impl::PropertyValueHolder;

    struct BuiltIns {
        static const std::string DEFAULT;
        static const std::string TEXTURE_ONLY;
        static const std::string DIFFUSE_ONLY;
    };

    static const std::unordered_map<std::string, std::string> BUILT_IN_NAMES;

    Material(MaterialID id, AssetManager *asset_manager);
    virtual ~Material() {}

    template<typename T>
    PropertyIndex define_property(
        MaterialPropertyType type,
        std::string name,
        std::string shader_variable,
        const T& default_value
    ) {
        assert(defined_property_count_ < _material_impl::MAX_DEFINED_PROPERTIES - 1);

        _material_impl::DefinedProperty& prop = defined_properties_[defined_property_count_++];
        assert(defined_property_count_ > 0);
        prop.index = defined_property_count_ - 1;
        prop.name = name;
        prop.type = type;
        prop.default_value.set(default_value);
        prop.shader_variable = shader_variable;
        set_property_value(prop.index, default_value);
        defined_property_lookup_.insert(std::make_pair(name, prop.index));

        if(type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
            texture_properties_.push_back(prop.index);
        }

        assert(!name.empty());

        return prop.index;
    }

    PropertyIndex defined_property_index(const std::string& name) const {
        return defined_property_lookup_.at(name);
    }

    MaterialPropertyType defined_property_type(const std::string& name) const {
        return defined_properties_[defined_property_index(name)].type;
    }

    const std::vector<PropertyIndex> &defined_properties_by_type(MaterialPropertyType type) const;
    std::vector<std::string> defined_custom_properties() const;

    bool property_is_defined(const std::string& name) const {
        return defined_property_lookup_.count(name) > 0;
    }

// ---------- Passes ------------------------
    void set_pass_count(uint8_t pass_count);

    uint8_t pass_count() const {
        return pass_count_;
    }

    MaterialPass* pass(uint8_t pass);

    void each(std::function<void (uint32_t, MaterialPass*)> callback) {
        for(auto i = 0; i != pass_count_; ++i) {
            callback(i, &passes_[i]);
        }
    }

    void update(float dt);

private:
    void initialize_default_properties();

    uint32_t defined_property_count_ = 0;
    _material_impl::DefinedProperty defined_properties_[_material_impl::MAX_DEFINED_PROPERTIES];
    std::unordered_map<std::string, uint32_t> defined_property_lookup_;

    std::vector<PropertyIndex> texture_properties_;

    std::mutex pass_mutex_;
    uint8_t pass_count_ = 0;
    MaterialPass passes_[_material_impl::MAX_PASSES];


    /* Assignment operator and copy constructor must be private
     * to prevent accidental copying. However the object manager needs
     * to be able to clone materials, hence the friendship.
     */

    template<typename T>
    PropertyIndex define_builtin_property(
        MaterialPropertyType type,
        std::string name,
        std::string shader_variable,
        const T& default_value
    ) {
        auto index = define_property(type, name, shader_variable, default_value);
        defined_properties_[index].is_custom = false;
        return index;
    }

    /* These indexes exist for performance. It saves a map lookup up for each of these properties */
    PropertyIndex material_ambient_index_;
    PropertyIndex material_diffuse_index_;
    PropertyIndex material_specular_index_;
    PropertyIndex material_shininess_index_;
    PropertyIndex diffuse_map_index_;
    PropertyIndex specular_map_index_;
    PropertyIndex light_map_index_;
    PropertyIndex normal_map_index_;

    PropertyIndex blending_enabled_index_;
    PropertyIndex texturing_enabled_index_;
    PropertyIndex lighting_enabled_index_;
    PropertyIndex depth_test_enabled_index_;
    PropertyIndex depth_write_enabled_index_;

    PropertyIndex shade_model_index_;
    PropertyIndex cull_mode_index_;
    PropertyIndex polygon_mode_index_;
    PropertyIndex point_size_index_;
    PropertyIndex colour_material_index_;
    PropertyIndex blend_func_index_;
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
    return defined_property_->values[slot_].get<T>();
}

namespace _material_impl {

template<typename T>
void PropertyValueHolder::set_property_value(uint32_t index, const T& value) {
    _material_impl::DefinedProperty* defined_property = &top_level_->defined_properties_[index];

    assert(defined_property->type == _material_impl::MaterialTypeForType<T>::type);

    defined_property->values[slot_].set(value);
    defined_property->slot_used[slot_] = true;
}


template<typename T>
void PropertyValueHolder::set_property_value(const std::string& name, const T& value) {
    set_property_value(top_level()->defined_property_index(name), value);
}

}

}

#endif // MATERIAL_H
