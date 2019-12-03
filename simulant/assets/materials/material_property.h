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

namespace smlt {

typedef int16_t MaterialPropertyID;
const MaterialPropertyID MATERIAL_PROPERTY_ID_INVALID = -1;

class MaterialVariant {
public:
    template<typename T>
    T& get() const;

    void set(int v);
    void set(float v);
    void set(const Vec2& v);
    void set(const Vec3& v);
    void set(const Vec4& v);
    void set(const Mat3& v);
    void set(const Mat4& v);
    void set(const TextureUnit& v);
};


class MaterialObject;

class MaterialProperty {
public:
    MaterialPropertyID id;
    MaterialPropertyType type;
    MaterialVariant default_value;
    bool is_custom = true;
};


class MaterialPropertyValue {
public:
    template<typename T>
    T& value() {
        return variant_.get<T>();
    }

    std::string shader_variable() const;
    std::string name() const;
    bool is_custom() const;
    MaterialPropertyType type() const;
private:
    MaterialVariant variant_;
};

}
