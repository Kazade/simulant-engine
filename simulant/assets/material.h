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
#include <set>

#include "../asset.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../types.h"
#include "../loadable.h"
#include "../interfaces/updateable.h"

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

class Material:
    public Asset,
    public Loadable,
    public generic::Identifiable<AssetID>,
    public RefCounted<Material>,
    public Updateable,
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

    void update(float dt) override;

    const std::unordered_map<MaterialPropertyNameHash, MaterialPropertyType>& custom_properties() const {
        return custom_properties_;
    }

    const std::set<MaterialPropertyNameHash>& texture_properties() const {
        return texture_properties_;
    }

    /* Return the string name of a property, given its name hash */
    bool property_name(MaterialPropertyNameHash hsh, std::string& name) const {
        auto it = hashes_to_names_.find(hsh);
        if(it != hashes_to_names_.end()) {
            name = it->second.name;
            return true;
        } else {
            return false;
        }
    }

private:
    Renderer* renderer_ = nullptr;
    std::vector<MaterialPass> passes_;

    struct PropertyName {
        std::string name;
        uint8_t ref_count = 0;
    };

    static std::unordered_map<
        MaterialPropertyNameHash,
        PropertyName
    > hashes_to_names_;

    static uint32_t _name_refcount(const char* name) {
        auto hsh = material_property_hash(name);

        auto it = hashes_to_names_.find(hsh);
        if(it != hashes_to_names_.end()) {
            return it->second.ref_count;
        } else {
            return 0;
        }
    }

    /* Push a name onto the hash lookup */
    static PropertyName* push_name(const char* name, MaterialPropertyNameHash hsh) {
        auto it = hashes_to_names_.find(hsh);
        if(it != hashes_to_names_.end()) {
            it->second.ref_count++;
            return &it->second;
        } else {
            PropertyName pn;
            pn.name = name;
            pn.ref_count = 1;
            hashes_to_names_[hsh] = pn;
            return &hashes_to_names_[hsh];
        }
    }

    /* Reduce the refcount of hsh. Returns true if this was the
     * last hsh entry */
    static bool pop_name(MaterialPropertyNameHash hsh) {
        auto it = hashes_to_names_.find(hsh);
        if(it != hashes_to_names_.end()) {
            it->second.ref_count--;
            if(!it->second.ref_count) {
                hashes_to_names_.erase(it);
                return true;
            }
        }

        return false;
    }

    std::set<MaterialPropertyNameHash> texture_properties_;

    std::unordered_map<
        MaterialPropertyNameHash,
        MaterialPropertyType
    > custom_properties_;

    virtual void on_override(
        MaterialPropertyNameHash hsh,
        const char *name,
        MaterialPropertyType type) override {

        push_name(name, hsh);

        if(type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
            texture_properties_.insert(hsh);
        }

        if(!is_core_property(hsh)) {
            custom_properties_[hsh] = type;
        }
    }

    virtual void on_clear_override(MaterialPropertyNameHash hsh) override {
        pop_name(hsh);
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
