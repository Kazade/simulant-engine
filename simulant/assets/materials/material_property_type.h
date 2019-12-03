#pragma once

#include "../../types.h"

namespace smlt {

enum MaterialPropertyType {
    MATERIAL_PROPERTY_TYPE_INVALID = -1,
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

/* Value type, if the type is texture */
struct TextureUnit {
    friend class ::MaterialTest;

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
    std::shared_ptr<Texture> texture_;
};


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

}
