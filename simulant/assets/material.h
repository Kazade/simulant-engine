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
#include "../types.h"
#include "../loadable.h"

#include "materials/material_object.h"
#include "materials/constants.h"

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

    MaterialPass();

    MaterialPass(Material* material);

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

private:
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
        return passes_.size();
    }

    MaterialPass* pass(uint8_t pass);

    void each(std::function<void (uint32_t, MaterialPass*)> callback) {
        for(std::size_t i = 0; i != passes_.size(); ++i) {
            callback(i, &passes_[i]);
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
    std::vector<MaterialPass> passes_;

    std::unordered_map<MaterialPropertyNameHash, TexturePropertyInfo> texture_properties_;

    std::unordered_map<
        MaterialPropertyNameHash,
        CustomPropertyInfo
    > custom_properties_;

    virtual void on_override(
        MaterialPropertyNameHash hsh,
        const char *name,
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

    void on_clear_override(MaterialPropertyNameHash hsh) override {
        texture_properties_.erase(hsh);
        custom_properties_.erase(hsh);
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
};


}

#endif // MATERIAL_H
