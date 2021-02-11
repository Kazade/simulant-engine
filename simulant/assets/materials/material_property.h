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

typedef FastVariant<bool, int, float, Vec2, Vec3, Vec4, Mat3, Mat4, TextureUnit> MaterialVariant;

const MaterialPropertyID MATERIAL_PROPERTY_ID_INVALID = -1;

namespace _material_impl {

template<typename T>
struct TypeToInt;

template<>
struct TypeToInt<bool> {
    const static int value = 0;
};

template<>
struct TypeToInt<int> {
    const static int value = 1;
};

template<>
struct TypeToInt<float> {
    const static int value = 2;
};

template<>
struct TypeToInt<Vec2> {
    const static int value = 3;
};

template<>
struct TypeToInt<Vec3> {
    const static int value = 4;
};

template<>
struct TypeToInt<Vec4> {
    const static int value = 5;
};

template<>
struct TypeToInt<Mat3> {
    const static int value = 6;
};

template<>
struct TypeToInt<Mat4> {
    const static int value = 7;
};

template<>
struct TypeToInt<TextureUnit> {
    const static int value = 8;
};

}

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

    MaterialPropertyValue(const MaterialPropertyValue& rhs) {
        (*this) = rhs;
    }

    MaterialPropertyValue& operator=(const MaterialPropertyValue& rhs) {
        delete_data();

        switch(rhs.stored_type_) {
            case -1: {
                stored_type_ = -1;
                data_ = nullptr;
            } break;
            case _material_impl::TypeToInt<bool>::value: {
                set_value(rhs.value<bool>());
            } break;
            case _material_impl::TypeToInt<int>::value: {
                set_value(rhs.value<int>());
            } break;
            case _material_impl::TypeToInt<float>::value: {
                set_value(rhs.value<float>());
            } break;
            case _material_impl::TypeToInt<Vec2>::value: {
                set_value(rhs.value<Vec2>());
            } break;
            case _material_impl::TypeToInt<Vec3>::value: {
                set_value(rhs.value<Vec3>());
            } break;
            case _material_impl::TypeToInt<Vec4>::value: {
                set_value(rhs.value<Vec4>());
            } break;
            case _material_impl::TypeToInt<Mat3>::value: {
                set_value(rhs.value<Mat3>());
            } break;
            case _material_impl::TypeToInt<Mat4>::value: {
                set_value(rhs.value<Mat4>());
            } break;
            case _material_impl::TypeToInt<TextureUnit>::value: {
                set_value(rhs.value<TextureUnit>());
            } break;
            default:
                FATAL_ERROR(
                    ERROR_CODE_INVALID_TYPE_ERROR,
                    "Invalid type used in material property"
                );
        }

        return *this;
    }

    template<typename T>
    MaterialPropertyValue(MaterialProperty* property, const T& value):
        property_(property) {

        set_value(value);
    }

    ~MaterialPropertyValue() {
        delete_data();
    }

    template<typename T>
    const T& value() const {
        assert(_material_impl::TypeToInt<T>::value == stored_type_);
        assert(data_);

        return *((T*) data_);
    }

    template<typename T>
    T& value() {
        assert(_material_impl::TypeToInt<T>::value == stored_type_);
        assert(data_);

        return *((T*) data_);
    }

    std::string shader_variable() const;
    std::string name() const;
    bool is_custom() const;
    MaterialPropertyType type() const;

private:
    template<typename T>
    void set_value(const T& v) {
        if(stored_type_ > -1) {
            delete_data();
        }

        assert(!data_);

        stored_type_ = _material_impl::TypeToInt<T>::value;
        T* new_thing = new T(v);
        data_ = new_thing;
    }

    void delete_data() {
        if(stored_type_== -1) {
            assert(!data_);
            return;
        }

        switch(stored_type_) {
            case _material_impl::TypeToInt<bool>::value: {
                bool* d = static_cast<bool*>(data_);
                delete d;
            } break;
            case _material_impl::TypeToInt<int>::value: {
                int* i = static_cast<int*>(data_);
                delete i;
            } break;
            case _material_impl::TypeToInt<float>::value: {
                float* f = static_cast<float*>(data_);
                delete f;
            } break;
            case _material_impl::TypeToInt<Vec2>::value: {
                Vec2* v = static_cast<Vec2*>(data_);
                delete v;
            } break;
            case _material_impl::TypeToInt<Vec3>::value: {
                Vec3* v = static_cast<Vec3*>(data_);
                delete v;
            } break;
            case _material_impl::TypeToInt<Vec4>::value: {
                Vec4* v = static_cast<Vec4*>(data_);
                delete v;
            } break;
            case _material_impl::TypeToInt<Mat3>::value: {
                Mat3* v = static_cast<Mat3*>(data_);
                delete v;
            } break;
            case _material_impl::TypeToInt<Mat4>::value: {
                Mat4* v = static_cast<Mat4*>(data_);
                delete v;
            } break;
            case _material_impl::TypeToInt<TextureUnit>::value: {
                TextureUnit* v = static_cast<TextureUnit*>(data_);
                delete v;
            } break;
            default:
                FATAL_ERROR(
                    ERROR_CODE_INVALID_TYPE_ERROR,
                    "Invalid type used in material property"
                );
        }

        data_ = nullptr;
        stored_type_ = -1;
    }

    MaterialProperty* property_ = nullptr;

    int stored_type_ = -1;
    void* data_ = nullptr;
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

