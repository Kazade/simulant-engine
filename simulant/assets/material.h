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

#include <list>
#include <unordered_map>
#include <unordered_set>

#include "../asset.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../loadable.h"
#include "../types.h"
#include "../utils/limited_vector.h"
#include "materials/constants.h"
#include "materials/material_object.h"
#include "materials/core/core_material_props.h"

namespace smlt {

class Renderer;

enum IterationType {
    ITERATION_TYPE_ONCE,
    ITERATION_TYPE_N,
    ITERATION_TYPE_ONCE_PER_LIGHT
};

class MaterialPass:
    public MaterialObject  {
public:
    friend class Material;

    using MaterialPropertyOverrider::property_value;
    using MaterialPropertyOverrider::set_property_value;

    MaterialPass();

    void set_iteration_type(IterationType iteration) {
        iteration_type_ = iteration;
    }

    IterationType iteration_type() const {
        return iteration_type_;
    }

    GPUProgramID gpu_program_id() const;

    void set_gpu_program(GPUProgramPtr program) {
        program_ = program;
    }

    uint8_t max_iterations() const {
        return max_iterations_;
    }

    const Material* material() const {
        return material_;
    }

    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const bool& value) override  {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const float& value) override  {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const int32_t& value) override  {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec2& value) override  {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec3& value) override  {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec4& value) override  {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Mat3& value) override  {
        return _set_property_value<Mat3>(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Mat4& value) override  {
        return _set_property_value<Mat4>(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const TexturePtr& value) override {
        return _set_property_value<TexturePtr>(hsh, name, value);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const bool*& out) const override  {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const float*& out) const override  {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const int32_t*& out) const override  {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec2*& out) const override  {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec3*& out) const override  {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec4*& out) const override  {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Mat3*& out) const override  {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Mat4*& out) const override {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const TexturePtr*& out) const override {
        return _property_value(hsh, out);
    }

    template<typename T>
    bool _property_value(const MaterialPropertyNameHash hsh,
                         const T*& out) const;
    template<typename T>
    bool _set_property_value(MaterialPropertyNameHash hsh, const char* name,
                             const T& value);

    bool on_clear_override(MaterialPropertyNameHash hsh) override;
    bool on_check_existence(MaterialPropertyNameHash hsh) const override;

    bool property_type(const char* property_name,
                       MaterialPropertyType* type) const override;

    /* Named accessors shadowing MaterialObject — bypass virtual dispatch
     * when called through a MaterialPass* (defined after Material below). */
    const Color& base_color() const;
    const Color& fog_color() const;
    float metallic() const;
    float roughness() const;
    float point_size() const;
    float alpha_threshold() const;
    float fog_density() const;
    float fog_start() const;
    float fog_end() const;
    int32_t textures_enabled() const;
    BlendType blend_func() const;
    bool is_blending_enabled() const;
    DepthFunc depth_func() const;
    CullMode cull_mode() const;
    ShadeModel shade_model() const;
    ColorMaterial color_material() const;
    PolygonMode polygon_mode() const;
    FogMode fog_mode() const;
    bool is_depth_write_enabled() const;
    bool is_depth_test_enabled() const;
    bool is_lighting_enabled() const;
    bool is_stencil_test_enabled() const;
    bool is_color_write_enabled() const;
    StencilFunc stencil_func() const;
    int32_t stencil_ref() const;
    int32_t stencil_mask() const;
    StencilOp stencil_fail_op() const;
    StencilOp stencil_depth_fail_op() const;
    StencilOp stencil_pass_op() const;
    PolygonListTarget polygon_list_target() const;

    void set_base_color(const Color& v);
    void set_fog_color(const Color& v);
    void set_metallic(float v);
    void set_roughness(float v);
    void set_alpha_threshold(float v);
    void set_fog_density(float v);
    void set_fog_start(float v);
    void set_fog_end(float v);
    void set_textures_enabled(EnabledTextureMask v);
    void set_blend_func(BlendType v);
    void set_depth_func(DepthFunc v);
    void set_cull_mode(CullMode v);
    void set_shade_model(ShadeModel v);
    void set_color_material(ColorMaterial v);
    void set_polygon_mode(PolygonMode v);
    void set_fog_mode(FogMode v);
    void set_depth_write_enabled(bool v);
    void set_depth_test_enabled(bool v);
    void set_lighting_enabled(bool v);
    void set_stencil_test_enabled(bool v);
    void set_color_write_enabled(bool v);
    void set_stencil_func(StencilFunc func, int32_t ref = 0, int32_t mask = 0xFF);
    void set_stencil_ops(StencilOp fail, StencilOp depth_fail, StencilOp pass);
    void set_polygon_list_target(PolygonListTarget v);

private:
    MaterialPass(Material* material, uint8_t pass_number);

    uint8_t pass_number_ = 0;
    IterationType iteration_type_ = ITERATION_TYPE_ONCE;
    uint8_t max_iterations_ = 1;
    Material* material_ = nullptr;
    GPUProgramPtr program_ = nullptr;

    /* Flat override storage — no pool, no pointer chain. */
    CoreMaterialProps pass_props_;
    uint32_t override_mask_ = 0;
};

typedef uint8_t PropertyIndex;

struct TexturePropertyInfo {
    std::string texture_property_name;
    std::string matrix_property_name;
    MaterialPropertyNameHash texture_property_name_hash;
    MaterialPropertyNameHash matrix_property_name_hash;
};

struct CustomPropertyInfo {
    std::string property_name;
    MaterialPropertyNameHash property_name_hash;
    MaterialPropertyType type;
};

class Material:
    public Asset,
    public Loadable,
    public generic::Identifiable<AssetID>,
    public RefCounted<Material>,
    public MaterialObject,
    public ChainNameable<Material> {

public:
    friend class GenericRenderer;
    friend class MaterialPass;

    struct BuiltIns {
        static const std::string DEFAULT;
        static const std::string TEXTURE_ONLY;
        static const std::string DIFFUSE_ONLY;
    };

    static const std::unordered_map<std::string, std::string> BUILT_IN_NAMES;

    Material(AssetID id, AssetManager *asset_manager);
    virtual ~Material();

// ---------- Passes ------------------------
    bool set_pass_count(uint8_t pass_count);

    uint8_t pass_count() const {
        return (uint8_t)passes_.size();
    }

    MaterialPass* pass(uint8_t pass);

    void each(std::function<void (uint32_t, MaterialPass*)> callback) {
        for(std::size_t i = 0; i != passes_.size(); ++i) {
            callback((uint32_t)i, &passes_[i]);
        }
    }

    const std::unordered_map<MaterialPropertyNameHash, CustomPropertyInfo>& custom_properties() const {
        return custom_properties_;
    }

    const std::unordered_map<MaterialPropertyNameHash, TexturePropertyInfo>& texture_properties() const {
        return texture_properties_;
    }

private:
    Renderer* renderer_ = nullptr;
    LimitedVector<MaterialPass, MAX_MATERIAL_PASSES> passes_;

    /* Flat storage for all core scalar/bool/Color/enum properties.
     * Eliminates pool pointer chain (4 pointer hops) for hot render-state fields. */
    CoreMaterialProps base_props_;

    struct MaterialPropertyEntry {
        MaterialPropertyNameHash hsh = 0;
        MaterialPropertyValuePointer entries[MAX_MATERIAL_PASSES];
        MaterialPropertyEntry* next = nullptr;
    };

    static constexpr int bucket_count = 16;
    std::array<MaterialPropertyEntry, bucket_count> values_;

    MaterialPropertyEntry* find_entry(MaterialPropertyNameHash hsh) {
        auto it = &values_[hsh % Material::bucket_count];

        while(it->hsh != hsh && it->next) {
            it = it->next;
        }

        if(it->hsh == hsh) {
            return it;
        }

        return nullptr;
    }

    const MaterialPropertyEntry*
        find_entry(MaterialPropertyNameHash hsh) const {
        auto it = &values_[hsh % Material::bucket_count];

        while(it->hsh != hsh && it->next) {
            it = it->next;
        }

        if(it->hsh == hsh) {
            return it;
        }

        return nullptr;
    }

    MaterialPropertyEntry*
        find_entry_or_last_in_bucket(MaterialPropertyNameHash hsh) {
        auto it = &values_[hsh % Material::bucket_count];

        while(it->hsh != hsh && it->next) {
            it = it->next;
        }

        return it;
    }

    std::unordered_map<MaterialPropertyNameHash, TexturePropertyInfo>
        texture_properties_;

    std::unordered_map<MaterialPropertyNameHash, CustomPropertyInfo>
        custom_properties_;

    virtual void on_override(MaterialPropertyNameHash hsh, const char* name,
                             MaterialPropertyType type) override {

        if(type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
            TexturePropertyInfo info;
            info.texture_property_name = name;
            info.texture_property_name_hash = hsh;
            info.matrix_property_name = info.texture_property_name + "_matrix";
            info.matrix_property_name_hash =
                material_property_hash(info.matrix_property_name.c_str());
            texture_properties_[info.texture_property_name_hash] = info;
        }

        if(!is_core_property(hsh)) {
            CustomPropertyInfo info;
            info.property_name = name;
            info.property_name_hash = hsh;
            info.type = type;
            custom_properties_[hsh] = info;
        }
    }

    bool on_clear_override(MaterialPropertyNameHash hsh) override {
        texture_properties_.erase(hsh);
        custom_properties_.erase(hsh);

        auto it = find_entry(hsh);
        if(it && it->entries[0]) {
            it->entries[0].reset();
            return true;
        }

        return false;
    }

protected:
    /* Assignment operator and copy constructor must be private
     * to prevent accidental copying. However the object manager needs
     * to be able to clone materials, hence the friendship.
     */

    friend class _object_manager_impl::ObjectManagerBase<
        AssetID, Material, std::shared_ptr<smlt::Material>,
        _object_manager_impl::ToSharedPtr<smlt::Material>
    >;

    Material(const Material& rhs) = delete;

    Material& operator=(const Material& rhs);

    void initialize_core_properties();

public:
    using MaterialPropertyOverrider::property_value;
    using MaterialPropertyOverrider::set_property_value;

    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const bool& value) override {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const float& value) override {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const int32_t& value) override {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec2& value) override {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec3& value) override {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec4& value) override {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Mat3& value) override {
        return _set_property_value<Mat3>(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Mat4& value) override {
        return _set_property_value<Mat4>(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const TexturePtr& value) override {
        return _set_property_value<TexturePtr>(hsh, name, value);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const bool*& out) const override {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const float*& out) const override {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const int32_t*& out) const override {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec2*& out) const override {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec3*& out) const override {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec4*& out) const override {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Mat3*& out) const override {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Mat4*& out) const override {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const TexturePtr*& out) const override {
        return _property_value(hsh, out);
    }

    /* Named accessors shadowing MaterialObject — direct struct read,
     * no virtual dispatch, no hash lookup, no pool pointer chain. */
    const Color& base_color() const     { return base_props_.base_color; }
    const Color& fog_color() const      { return base_props_.fog_color; }
    float metallic() const              { return base_props_.metallic; }
    float roughness() const             { return base_props_.roughness; }
    float point_size() const            { return base_props_.point_size; }
    float alpha_threshold() const       { return base_props_.alpha_threshold; }
    float fog_density() const           { return base_props_.fog_density; }
    float fog_start() const             { return base_props_.fog_start; }
    float fog_end() const               { return base_props_.fog_end; }
    int32_t textures_enabled() const    { return base_props_.textures_enabled; }
    BlendType blend_func() const        { return (BlendType)base_props_.blend_func; }
    bool is_blending_enabled() const    { return base_props_.blend_func != (int32_t)BLEND_NONE; }
    DepthFunc depth_func() const        { return (DepthFunc)base_props_.depth_func; }
    CullMode cull_mode() const          { return (CullMode)base_props_.cull_mode; }
    ShadeModel shade_model() const      { return (ShadeModel)base_props_.shade_model; }
    ColorMaterial color_material() const{ return (ColorMaterial)base_props_.color_material; }
    PolygonMode polygon_mode() const    { return (PolygonMode)base_props_.polygon_mode; }
    FogMode fog_mode() const            { return (FogMode)base_props_.fog_mode; }
    bool is_depth_write_enabled() const { return base_props_.depth_write_enabled; }
    bool is_depth_test_enabled() const  { return base_props_.depth_test_enabled; }
    bool is_lighting_enabled() const    { return base_props_.lighting_enabled; }

    void set_base_color(const Color& v)        { base_props_.base_color = v; }
    void set_fog_color(const Color& v)         { base_props_.fog_color = v; }
    void set_metallic(float v)                 { base_props_.metallic = v; }
    void set_roughness(float v)                { base_props_.roughness = v; }
    void set_alpha_threshold(float v)          { base_props_.alpha_threshold = v; }
    void set_fog_density(float v)              { base_props_.fog_density = v; }
    void set_fog_start(float v)                { base_props_.fog_start = v; }
    void set_fog_end(float v)                  { base_props_.fog_end = v; }
    void set_textures_enabled(EnabledTextureMask v){ base_props_.textures_enabled = (int32_t)v; }
    void set_blend_func(BlendType v)           { base_props_.blend_func = (int32_t)v; }
    void set_depth_func(DepthFunc v)           { base_props_.depth_func = (int32_t)v; }
    void set_cull_mode(CullMode v)             { base_props_.cull_mode = (int32_t)v; }
    void set_shade_model(ShadeModel v)         { base_props_.shade_model = (int32_t)v; }
    void set_color_material(ColorMaterial v)   { base_props_.color_material = (int32_t)v; }
    void set_polygon_mode(PolygonMode v)       { base_props_.polygon_mode = (int32_t)v; }
    void set_fog_mode(FogMode v)               { base_props_.fog_mode = (int32_t)v; }
    void set_depth_write_enabled(bool v)       { base_props_.depth_write_enabled = v; }
    void set_depth_test_enabled(bool v)        { base_props_.depth_test_enabled = v; }
    void set_lighting_enabled(bool v)          { base_props_.lighting_enabled = v; }

    template<typename T>
    bool _property_value(const MaterialPropertyNameHash hsh,
                         const T*& out) const {
        /* Core scalar/bool/Color/enum: direct struct read, no pool */
        const T* core = core_scalar_ptr<T>(base_props_, hsh);
        if(core) { out = core; return true; }

        /* TexturePtr, Mat4, custom properties remain in the pool */
        auto it = find_entry(hsh);
        if(it && it->entries[0]) {
            out = it->entries[0].get<T>();
            return true;
        }

        return false;
    }

    MaterialValuePool* _get_pool() const;

    template<typename T>
    bool _set_property_value(MaterialPropertyNameHash hsh, const char* name,
                             const T& value) {
        /* Core scalar/bool/Color/enum: write directly to struct */
        T* core = core_scalar_mutable_ptr<T>(base_props_, hsh);
        if(core) { *core = value; return false; }

        /* TexturePtr, Mat4, custom properties use the pool */
        auto property_value_ptr = _get_pool()->get_or_create_value(value);
        auto it = find_entry_or_last_in_bucket(hsh);
        bool ret = false;
        if(it && it->hsh == hsh) {
            clear_override(hsh);
            it->entries[0] = property_value_ptr;
        } else {
            auto entry = new MaterialPropertyEntry();
            entry->hsh = hsh;
            entry->entries[0] = property_value_ptr;
            it->next = entry;
            ret = true;
        }

        on_override(hsh, name, property_value_ptr.type());
        return ret;
    }

    bool property_type(const char* name,
                       MaterialPropertyType* type) const override {
        auto hsh = material_property_hash(name);

        /* Core scalar/bool/Color/enum properties live in base_props_, not pool */
        if(core_property_type(hsh, type)) {
            return true;
        }

        auto it = find_entry(hsh);
        if(it && it->entries[0]) {
            *type = it->entries[0].type();
            return true;
        }

        return false;
    }

    bool on_check_existence(MaterialPropertyNameHash hsh) const override {
        /* Core properties always exist */
        if(core_mask_for_hash(hsh)) return true;
        auto it = find_entry(hsh);
        return it && it->entries[0];
    }
};

inline bool MaterialPass::on_clear_override(MaterialPropertyNameHash hsh) {
    uint32_t bit = core_mask_for_hash(hsh);
    if(bit) {
        if(override_mask_ & bit) {
            override_mask_ &= ~bit;
            return true;
        }
        return false;
    }
    auto material = (Material*)parent_;
    auto it = material->find_entry(hsh);
    if(it && it->entries[pass_number_ + 1]) {
        it->entries[pass_number_ + 1].reset();
        return true;
    }
    return false;
}

inline bool MaterialPass::on_check_existence(MaterialPropertyNameHash hsh) const {
    uint32_t bit = core_mask_for_hash(hsh);
    if(bit) {
        return (override_mask_ & bit) != 0;
    }
    auto material = (Material*)parent_;
    auto it = material->find_entry(hsh);
    return it && it->entries[pass_number_ + 1];
}

template<typename T>
bool MaterialPass::_set_property_value(MaterialPropertyNameHash hsh,
                                       const char* name, const T& value) {
    /* Core scalar/bool/Color/enum: write to pass struct and set mask bit */
    T* core = core_scalar_mutable_ptr<T>(pass_props_, hsh);
    if(core) {
        *core = value;
        override_mask_ |= core_mask_for_hash(hsh);
        return false;
    }

    /* TexturePtr, Mat4, custom: use pool on parent Material */
    clear_override(hsh);

    auto material = (Material*)parent_;
    auto it = material->find_entry(hsh);
    if(it == nullptr) {
        return false;
    }

    auto property_value_ptr = material->_get_pool()->get_or_create_value(value);
    it->entries[pass_number_ + 1] = property_value_ptr;
    on_override(hsh, name, property_value_ptr.type());
    return true;
}

template<typename T>
bool MaterialPass::_property_value(const MaterialPropertyNameHash hsh,
                                   const T*& out) const {
    /* Core scalar/bool/Color/enum: direct struct read, no pool */
    uint32_t mask = core_mask_for_hash(hsh);
    if(mask) {
        if(override_mask_ & mask) {
            out = core_scalar_ptr<T>(pass_props_, hsh);
        } else {
            out = core_scalar_ptr<T>(material_->base_props_, hsh);
        }
        return out != nullptr;
    }

    /* TexturePtr, Mat4, custom: pool path */
    auto it = material()->find_entry(hsh);
    if(it) {
        if(it->entries[pass_number_ + 1]) {
            out = it->entries[pass_number_ + 1].get<T>();
            return true;
        }
        if(it->entries[0]) {
            out = it->entries[0].get<T>();
            return true;
        }
    }
    return false;
}

/* MaterialPass named accessor definitions (need Material to be complete).
 * These shadow MaterialObject methods — direct struct read, no virtual dispatch. */
inline const Color& MaterialPass::base_color() const {
    return (override_mask_ & CORE_MASK_BASE_COLOR) ? pass_props_.base_color : material_->base_props_.base_color;
}
inline const Color& MaterialPass::fog_color() const {
    return (override_mask_ & CORE_MASK_FOG_COLOR) ? pass_props_.fog_color : material_->base_props_.fog_color;
}
inline float MaterialPass::metallic() const {
    return (override_mask_ & CORE_MASK_METALLIC) ? pass_props_.metallic : material_->base_props_.metallic;
}
inline float MaterialPass::roughness() const {
    return (override_mask_ & CORE_MASK_ROUGHNESS) ? pass_props_.roughness : material_->base_props_.roughness;
}
inline float MaterialPass::point_size() const {
    return (override_mask_ & CORE_MASK_POINT_SIZE) ? pass_props_.point_size : material_->base_props_.point_size;
}
inline float MaterialPass::alpha_threshold() const {
    return (override_mask_ & CORE_MASK_ALPHA_THRESHOLD) ? pass_props_.alpha_threshold : material_->base_props_.alpha_threshold;
}
inline float MaterialPass::fog_density() const {
    return (override_mask_ & CORE_MASK_FOG_DENSITY) ? pass_props_.fog_density : material_->base_props_.fog_density;
}
inline float MaterialPass::fog_start() const {
    return (override_mask_ & CORE_MASK_FOG_START) ? pass_props_.fog_start : material_->base_props_.fog_start;
}
inline float MaterialPass::fog_end() const {
    return (override_mask_ & CORE_MASK_FOG_END) ? pass_props_.fog_end : material_->base_props_.fog_end;
}
inline int32_t MaterialPass::textures_enabled() const {
    return (override_mask_ & CORE_MASK_TEXTURES_ENABLED) ? pass_props_.textures_enabled : material_->base_props_.textures_enabled;
}
inline BlendType MaterialPass::blend_func() const {
    const int32_t v = (override_mask_ & CORE_MASK_BLEND_FUNC) ? pass_props_.blend_func : material_->base_props_.blend_func;
    return (BlendType)v;
}
inline bool MaterialPass::is_blending_enabled() const {
    return blend_func() != BLEND_NONE;
}
inline DepthFunc MaterialPass::depth_func() const {
    const int32_t v = (override_mask_ & CORE_MASK_DEPTH_FUNC) ? pass_props_.depth_func : material_->base_props_.depth_func;
    return (DepthFunc)v;
}
inline CullMode MaterialPass::cull_mode() const {
    const int32_t v = (override_mask_ & CORE_MASK_CULL_MODE) ? pass_props_.cull_mode : material_->base_props_.cull_mode;
    return (CullMode)v;
}
inline ShadeModel MaterialPass::shade_model() const {
    const int32_t v = (override_mask_ & CORE_MASK_SHADE_MODEL) ? pass_props_.shade_model : material_->base_props_.shade_model;
    return (ShadeModel)v;
}
inline ColorMaterial MaterialPass::color_material() const {
    const int32_t v = (override_mask_ & CORE_MASK_COLOR_MATERIAL) ? pass_props_.color_material : material_->base_props_.color_material;
    return (ColorMaterial)v;
}
inline PolygonMode MaterialPass::polygon_mode() const {
    const int32_t v = (override_mask_ & CORE_MASK_POLYGON_MODE) ? pass_props_.polygon_mode : material_->base_props_.polygon_mode;
    return (PolygonMode)v;
}
inline FogMode MaterialPass::fog_mode() const {
    const int32_t v = (override_mask_ & CORE_MASK_FOG_MODE) ? pass_props_.fog_mode : material_->base_props_.fog_mode;
    return (FogMode)v;
}
inline bool MaterialPass::is_depth_write_enabled() const {
    return (override_mask_ & CORE_MASK_DEPTH_WRITE) ? pass_props_.depth_write_enabled : material_->base_props_.depth_write_enabled;
}
inline bool MaterialPass::is_depth_test_enabled() const {
    return (override_mask_ & CORE_MASK_DEPTH_TEST) ? pass_props_.depth_test_enabled : material_->base_props_.depth_test_enabled;
}
inline bool MaterialPass::is_lighting_enabled() const {
    return (override_mask_ & CORE_MASK_LIGHTING) ? pass_props_.lighting_enabled : material_->base_props_.lighting_enabled;
}

inline void MaterialPass::set_base_color(const Color& v) {
    pass_props_.base_color = v; override_mask_ |= CORE_MASK_BASE_COLOR;
}
inline void MaterialPass::set_fog_color(const Color& v) {
    pass_props_.fog_color = v; override_mask_ |= CORE_MASK_FOG_COLOR;
}
inline void MaterialPass::set_metallic(float v) {
    pass_props_.metallic = v; override_mask_ |= CORE_MASK_METALLIC;
}
inline void MaterialPass::set_roughness(float v) {
    pass_props_.roughness = v; override_mask_ |= CORE_MASK_ROUGHNESS;
}
inline void MaterialPass::set_alpha_threshold(float v) {
    pass_props_.alpha_threshold = v; override_mask_ |= CORE_MASK_ALPHA_THRESHOLD;
}
inline void MaterialPass::set_fog_density(float v) {
    pass_props_.fog_density = v; override_mask_ |= CORE_MASK_FOG_DENSITY;
}
inline void MaterialPass::set_fog_start(float v) {
    pass_props_.fog_start = v; override_mask_ |= CORE_MASK_FOG_START;
}
inline void MaterialPass::set_fog_end(float v) {
    pass_props_.fog_end = v; override_mask_ |= CORE_MASK_FOG_END;
}
inline void MaterialPass::set_textures_enabled(EnabledTextureMask v) {
    pass_props_.textures_enabled = (int32_t)v; override_mask_ |= CORE_MASK_TEXTURES_ENABLED;
}
inline void MaterialPass::set_blend_func(BlendType v) {
    pass_props_.blend_func = (int32_t)v; override_mask_ |= CORE_MASK_BLEND_FUNC;
}
inline void MaterialPass::set_depth_func(DepthFunc v) {
    pass_props_.depth_func = (int32_t)v; override_mask_ |= CORE_MASK_DEPTH_FUNC;
}
inline void MaterialPass::set_cull_mode(CullMode v) {
    pass_props_.cull_mode = (int32_t)v; override_mask_ |= CORE_MASK_CULL_MODE;
}
inline void MaterialPass::set_shade_model(ShadeModel v) {
    pass_props_.shade_model = (int32_t)v; override_mask_ |= CORE_MASK_SHADE_MODEL;
}
inline void MaterialPass::set_color_material(ColorMaterial v) {
    pass_props_.color_material = (int32_t)v; override_mask_ |= CORE_MASK_COLOR_MATERIAL;
}
inline void MaterialPass::set_polygon_mode(PolygonMode v) {
    pass_props_.polygon_mode = (int32_t)v; override_mask_ |= CORE_MASK_POLYGON_MODE;
}
inline void MaterialPass::set_fog_mode(FogMode v) {
    pass_props_.fog_mode = (int32_t)v; override_mask_ |= CORE_MASK_FOG_MODE;
}
inline void MaterialPass::set_depth_write_enabled(bool v) {
    pass_props_.depth_write_enabled = v; override_mask_ |= CORE_MASK_DEPTH_WRITE;
}
inline void MaterialPass::set_depth_test_enabled(bool v) {
    pass_props_.depth_test_enabled = v; override_mask_ |= CORE_MASK_DEPTH_TEST;
}
inline void MaterialPass::set_lighting_enabled(bool v) {
    pass_props_.lighting_enabled = v; override_mask_ |= CORE_MASK_LIGHTING;
}
inline bool MaterialPass::is_stencil_test_enabled() const {
    return pass_props_.stencil_test_enabled;
}
inline bool MaterialPass::is_color_write_enabled() const {
    return pass_props_.color_write_enabled;
}
inline StencilFunc MaterialPass::stencil_func() const {
    return (StencilFunc)pass_props_.stencil_func;
}
inline int32_t MaterialPass::stencil_ref() const {
    return pass_props_.stencil_ref;
}
inline int32_t MaterialPass::stencil_mask() const {
    return pass_props_.stencil_mask;
}
inline StencilOp MaterialPass::stencil_fail_op() const {
    return (StencilOp)pass_props_.stencil_fail;
}
inline StencilOp MaterialPass::stencil_depth_fail_op() const {
    return (StencilOp)pass_props_.stencil_depth_fail;
}
inline StencilOp MaterialPass::stencil_pass_op() const {
    return (StencilOp)pass_props_.stencil_pass;
}
inline void MaterialPass::set_stencil_test_enabled(bool v) {
    pass_props_.stencil_test_enabled = v;
}
inline void MaterialPass::set_color_write_enabled(bool v) {
    pass_props_.color_write_enabled = v;
}
inline void MaterialPass::set_stencil_func(StencilFunc func, int32_t ref, int32_t mask) {
    pass_props_.stencil_func = (int32_t)func;
    pass_props_.stencil_ref  = ref;
    pass_props_.stencil_mask = mask;
}
inline void MaterialPass::set_stencil_ops(StencilOp fail, StencilOp depth_fail, StencilOp pass) {
    pass_props_.stencil_fail       = (int32_t)fail;
    pass_props_.stencil_depth_fail = (int32_t)depth_fail;
    pass_props_.stencil_pass       = (int32_t)pass;
}
inline PolygonListTarget MaterialPass::polygon_list_target() const {
    return (PolygonListTarget)pass_props_.polygon_list_target;
}
inline void MaterialPass::set_polygon_list_target(PolygonListTarget v) {
    pass_props_.polygon_list_target = (int32_t)v;
}
}

#endif // MATERIAL_H
