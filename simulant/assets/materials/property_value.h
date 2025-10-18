#pragma once

#include <cstdint>
#include "../../math/vec2.h"
#include "../../math/vec3.h"
#include "../../math/vec4.h"
#include "../../math/mat3.h"
#include "../../math/mat4.h"
#include "../../types.h"
#include "../../core/memory.h"

namespace smlt {

enum MaterialPropertyType : uint16_t {
    MATERIAL_PROPERTY_TYPE_BOOL,
    MATERIAL_PROPERTY_TYPE_INT,
    MATERIAL_PROPERTY_TYPE_FLOAT,
    MATERIAL_PROPERTY_TYPE_VEC2,
    MATERIAL_PROPERTY_TYPE_VEC3,
    MATERIAL_PROPERTY_TYPE_VEC4,
    MATERIAL_PROPERTY_TYPE_MAT3,
    MATERIAL_PROPERTY_TYPE_MAT4,
    MATERIAL_PROPERTY_TYPE_TEXTURE
};


namespace _impl {
    template<typename T>
    struct material_property_lookup;

    template<>
    struct material_property_lookup<bool> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_BOOL;
    };

    template<>
    struct material_property_lookup<int32_t> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_INT;
    };

    template<>
    struct material_property_lookup<float> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_FLOAT;
    };

    template<>
    struct material_property_lookup<Vec2> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC2;
    };

    template<>
    struct material_property_lookup<Vec3> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC3;
    };

    template<>
    struct material_property_lookup<Vec4> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC4;
    };

    template<>
    struct material_property_lookup<Mat3> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_MAT3;
    };

    template<>
    struct material_property_lookup<Mat4> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_MAT4;
    };

    template<>
    struct material_property_lookup<TexturePtr> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_TEXTURE;
    };
}

} // namespace smlt
