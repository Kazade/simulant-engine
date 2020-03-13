#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>

#include "../../math/vec2.h"
#include "../../math/vec3.h"
#include "../../math/vec4.h"
#include "../../math/mat3.h"
#include "../../math/mat4.h"

#include "constants.h"
#include "material_property_type.h"
#include "fast_variant.h"

namespace smlt {

const MaterialPropertyID MATERIAL_PROPERTY_ID_INVALID = -1;

typedef FastVariant<bool, int, float, Vec2, Vec3, Vec4, Mat3, Mat4, TextureUnit> MaterialVariant;

class MaterialObject;
struct MaterialProperty;
class MaterialPropertyRegistry;

class MaterialPropertyValue {
public:
    friend class Material;
    friend class MaterialObject;
    friend class MaterialPropertyRegistry;
    friend struct MaterialProperty;

    MaterialPropertyValue() = default;

    template<typename T>
    MaterialPropertyValue(MaterialProperty* property, const T& value):
        property_(property),
        variant_(value) {

    }

    template<typename T>
    const T& value() const {
        return variant_.get<T>();
    }

    std::string shader_variable() const;
    std::string name() const;
    bool is_custom() const;
    MaterialPropertyType type() const;

private:
    template<typename T>
    void set_value(const T& v) {
        variant_.set(v);
    }

    MaterialProperty* property_ = nullptr;
    MaterialVariant variant_;
};

struct MaterialPropertyValueEntry {
    const void* object = nullptr;
    bool is_set = false;
    MaterialPropertyValue value;
};

struct MaterialProperty {
    MaterialProperty(MaterialPropertyID id):
        id(id) {}

    std::string name;
    MaterialPropertyID id;
    MaterialPropertyType type;
    bool is_custom = true;

    /* Realistically the only objects are the passes and the material itself
     * the material is the registry and takes slot 0 */
    MaterialPropertyValueEntry entries[MAX_MATERIAL_PASSES + 1];

    const MaterialPropertyValue* value(const MaterialObject* obj) const;
    MaterialPropertyValue* value(const MaterialObject* obj);

    template<typename T>
    void set_value(const MaterialObject* object, const T& value);

private:
    friend class MaterialPropertyRegistry;

    void init_entry(const MaterialObject* object);
    void release_entry(const MaterialObject* object);
};

}

