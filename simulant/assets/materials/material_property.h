#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>

#include "../../math/vec2.h"
#include "../../math/vec3.h"
#include "../../math/vec4.h"
#include "../../math/mat3.h"
#include "../../math/mat4.h"

#include "material_property_type.h"
#include "fast_variant.h"

namespace smlt {

typedef int16_t MaterialPropertyID;
const MaterialPropertyID MATERIAL_PROPERTY_ID_INVALID = -1;

typedef FastVariant<bool, int, float, Vec2, Vec3, Vec4, Mat3, Mat4, TextureUnit> MaterialVariant;

class MaterialObject;
class MaterialPropertyRegistry;

class MaterialPropertyValue {
public:
    friend class MaterialObject;
    friend class MaterialPropertyRegistry;

    MaterialPropertyValue(MaterialPropertyRegistry* registry, MaterialPropertyID id):
        registry_(registry),
        id_(id) {

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
    MaterialPropertyRegistry* registry_ = nullptr;
    MaterialPropertyID id_;

    MaterialVariant variant_;
};

class MaterialProperty {
public:
    MaterialProperty(MaterialPropertyRegistry* registry, MaterialPropertyID id):
        id(id),
        default_value(MaterialPropertyValue(registry, id)) {}

    std::string name;
    MaterialPropertyID id;
    MaterialPropertyType type;
    MaterialPropertyValue default_value;
    bool is_custom = true;
};

}
