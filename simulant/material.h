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

#include "asset.h"
#include "generic/identifiable.h"
#include "generic/managed.h"
#include "types.h"
#include "loadable.h"
#include "interfaces/updateable.h"

#include "assets/materials/material_object.h"
#include "assets/materials/material_property_registry.h"


class MaterialTest;

namespace smlt {


enum IterationType {
    ITERATION_TYPE_ONCE,
    ITERATION_TYPE_N,
    ITERATION_TYPE_ONCE_PER_LIGHT
};

class MaterialPass:
    public MaterialObject  {
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

    IterationType iteration_type_ = ITERATION_TYPE_ONCE;
    uint8_t max_iterations_ = 1;

    GPUProgramPtr program_;
};

typedef uint8_t PropertyIndex;

class Material:
    public Asset,
    public Loadable,
    public generic::Identifiable<MaterialID>,
    public RefCounted<Material>,
    public Updateable,
    public MaterialObject,
    public MaterialPropertyRegistry {

public:
    friend class GenericRenderer;

    struct BuiltIns {
        static const std::string DEFAULT;
        static const std::string TEXTURE_ONLY;
        static const std::string DIFFUSE_ONLY;
    };

    static const std::unordered_map<std::string, std::string> BUILT_IN_NAMES;

    Material(MaterialID id, AssetManager *asset_manager);
    virtual ~Material() {}

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
    std::mutex pass_mutex_;
    uint8_t pass_count_ = 0;
    std::array<MaterialPass, _material_impl::MAX_PASSES> passes_;

protected:
    /* Assignment operator and copy constructor must be private
     * to prevent accidental copying. However the object manager needs
     * to be able to clone materials, hence the friendship.
     */

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


}

#endif // MATERIAL_H
