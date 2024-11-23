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
#include <unordered_set>

#include "../asset.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../loadable.h"
#include "../types.h"
#include "../utils/limited_vector.h"
#include "materials/constants.h"
#include "materials/material_object.h"

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

    const Material* material() const;

    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const bool& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const float& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const int32_t& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec2& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec3& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec4& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Mat3& value) {
        return _set_property_value<Mat3>(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Mat4& value) {
        return _set_property_value<Mat4>(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const TexturePtr& value) {
        return _set_property_value<TexturePtr>(hsh, name, value);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const bool*& out) const {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const float*& out) const {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const int32_t*& out) const {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec2*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec3*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec4*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Mat3*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Mat4*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const TexturePtr*& out) const {
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

private:    
    MaterialPass(Material* material, uint8_t pass_number);

    uint8_t pass_number_;
    IterationType iteration_type_ = ITERATION_TYPE_ONCE;
    uint8_t max_iterations_ = 1;

    GPUProgramPtr program_;
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

    ContiguousMap<MaterialPropertyNameHash, MaterialPropertyValuePointer>
        values_;

    std::unordered_map<MaterialPropertyNameHash, TexturePropertyInfo>
        texture_properties_;

    std::unordered_map<
        MaterialPropertyNameHash,
        CustomPropertyInfo
    > custom_properties_;

    virtual void on_override(MaterialPropertyNameHash hsh, const char* name,
                             MaterialPropertyType type) override {

        if(type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
            TexturePropertyInfo info;
            info.texture_property_name = name;
            info.texture_property_name_hash = hsh;
            info.matrix_property_name = info.texture_property_name + "_matrix";
            info.matrix_property_name_hash = material_property_hash(info.matrix_property_name.c_str());
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

        if(values_.count(hsh)) {
            values_.at(hsh).reset();
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
                            const bool& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const float& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const int32_t& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec2& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec3& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Vec4& value) {
        return _set_property_value(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Mat3& value) {
        return _set_property_value<Mat3>(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const Mat4& value) {
        return _set_property_value<Mat4>(hsh, name, value);
    }
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name,
                            const TexturePtr& value) {
        return _set_property_value<TexturePtr>(hsh, name, value);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const bool*& out) const {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const float*& out) const {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const int32_t*& out) const {
        return _property_value(hsh, out);
    }

    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec2*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec3*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Vec4*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Mat3*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const Mat4*& out) const {
        return _property_value(hsh, out);
    }
    bool property_value(const MaterialPropertyNameHash hsh,
                        const TexturePtr*& out) const {
        return _property_value(hsh, out);
    }

    template<typename T>
    bool _property_value(const MaterialPropertyNameHash hsh,
                         const T*& out) const {
        auto it = values_.find(hsh);
        if(it != values_.end() && it->second) {
            out = it->second.get<T>();
            return true;
        }

        return false;
    }

    MaterialValuePool* _get_pool() const;

    template<typename T>
    bool _set_property_value(MaterialPropertyNameHash hsh, const char* name,
                             const T& value) {

        clear_override(hsh);

        auto property_value_ptr = _get_pool()->get_or_create_value(value);

        bool ret = true;
        if(values_.count(hsh)) {
            values_.at(hsh) = property_value_ptr;
            ret = false;
        } else {
            values_.insert(hsh, property_value_ptr);
        }

        on_override(hsh, name, property_value_ptr.type());
        return ret;
    }

    bool property_type(const char* name,
                       MaterialPropertyType* type) const override {
        auto hsh = material_property_hash(name);

        auto it = values_.find(hsh);
        if(it != values_.end()) {
            *type = it->second.type();
            return true;
        }

        return false;
    }

    bool on_check_existence(MaterialPropertyNameHash hsh) const {
        return values_.count(hsh);
    }
};

template<typename T>
bool MaterialPass::_set_property_value(MaterialPropertyNameHash hsh,
                                       const char* name, const T& value) {

    clear_override(hsh);

    auto key = hsh + (pass_number_ + 1);
    auto material = (Material*)parent_;
    auto& values = material->values_;
    auto property_value_ptr = material->_get_pool()->get_or_create_value(value);

    if(values.count(key)) {
        values.at(key) = property_value_ptr;
    } else {
        values.insert(key, property_value_ptr);
    }

    on_override(hsh, name, property_value_ptr.type());

    return true;
}

template<typename T>
bool MaterialPass::_property_value(const MaterialPropertyNameHash hsh,
                                   const T*& out) const {
    auto key = hsh + (pass_number_ + 1);
    auto& values = material()->values_;

    // Look up the pass value first (which is the hash offset by
    // the pass number)
    auto it = values.find(key);
    if(it != values.end() && it->second) {
        out = it->second.get<T>();
        return true;
    }

    // If we get here, then there's no pass override and
    // we should look for the material value
    it = values.find(hsh);
    if(it != values.end() && it->second) {
        out = it->second.get<T>();
        return true;
    }

    return false;
}
}

#endif // MATERIAL_H
