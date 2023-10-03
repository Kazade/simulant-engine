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

enum MaterialPropertyType {
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

template<typename T = void> struct PropertyValue;

struct BasePropertyValue {
    template<typename T>
    T* get() const {
        T* result;
        do_get(&result);
        return result;
    }

    template<typename T>
    bool set(const T& value) {
        return do_set(value);
    }

    virtual bool has_value() const = 0;
    virtual void clear() = 0;

private:
    virtual void do_get(bool** result) const { *result = nullptr; }
    virtual void do_get(int32_t** result) const { *result = nullptr; }
    virtual void do_get(float** result) const { *result = nullptr; }
    virtual void do_get(Vec2** result) const { *result = nullptr; }
    virtual void do_get(Vec3** result) const { *result = nullptr; }
    virtual void do_get(Vec4** result) const { *result = nullptr; }
    virtual void do_get(Mat3** result) const { *result = nullptr; }
    virtual void do_get(Mat4** result) const { *result = nullptr; }
    virtual void do_get(TexturePtr** result) const { *result = nullptr; }

    virtual bool do_set(const bool& value) { S_WARN("Invalid value for property: {0}", value); return false; }
    virtual bool do_set(const int32_t& value) { S_WARN("Invalid value for property: {0}", value); return false; }
    virtual bool do_set(const float& value) { S_WARN("Invalid value for property: {0}", value); return false; }
    virtual bool do_set(const Vec2& value) { S_WARN("Invalid value for property: {0}", value); return false; }
    virtual bool do_set(const Vec3& value) { S_WARN("Invalid value for property: {0}", value); return false; }
    virtual bool do_set(const Vec4& value) { S_WARN("Invalid value for property: {0}", value); return false; }
    virtual bool do_set(const Mat3& value) { S_WARN("Invalid value for property: {0}", value); return false; }
    virtual bool do_set(const Mat4& value) { _S_UNUSED(value); S_WARN("Invalid value for property"); return false; }
    virtual bool do_set(const TexturePtr& value) { S_WARN("Invalid value for property: {0}", value); return false; }

    void do_get(Color** result) const {
        do_get((Vec4**) result);
    }

    void do_get(uint32_t** result) const {
        do_get((int32_t**) result);
    }

    bool do_set(const Color& value) { return do_set((const Vec4&) value); }

    /* FIXME: Support these types properly */
    bool do_set(const int16_t& value) { return do_set((int32_t) value); }
    bool do_set(const uint32_t& value) { return do_set((int32_t) value); }
    bool do_set(const uint16_t& value) { return do_set((int32_t) value); }

};

template<typename T>
struct PropertyValue : public BasePropertyValue {
    bool is_set_ = false;
    T data;

    PropertyValue() = default;
    PropertyValue(T* value) {
        set(value);
    }

    PropertyValue(const PropertyValue& rhs) = default;
    PropertyValue(PropertyValue&& rhs) noexcept :
        is_set_(rhs.is_set_),
        data(std::move(rhs.data)) {}

    PropertyValue& operator=(const PropertyValue& ) = default;

    void clear() override {
        is_set_ = false;
    }

    bool has_value() const override {
        return is_set_;
    }
private:
    void do_get(T** result) const override {
        *result = (T*) &data;
    }

    bool do_set(const T& value) override {
        data = value;
        is_set_ = true;
        return true;
    }
};

template<>
struct PropertyValue<void> : public BasePropertyValue {
    MaterialPropertyType type;
    uint8_t* data = nullptr;

    PropertyValue() = default;

    template<typename T>
    PropertyValue(T value) {
        set(value);
    }

    PropertyValue& operator=(const PropertyValue& rhs) {
        switch(rhs.type) {
            case MATERIAL_PROPERTY_TYPE_BOOL:
                set(*rhs.get<bool>());
            break;
                case MATERIAL_PROPERTY_TYPE_INT:
                set(*rhs.get<int32_t>());
            break;
                case MATERIAL_PROPERTY_TYPE_FLOAT:
                set(*rhs.get<float>());
            break;
            case MATERIAL_PROPERTY_TYPE_VEC2:
                set(*rhs.get<Vec2>());
            break;
            case MATERIAL_PROPERTY_TYPE_VEC3:
                set(*rhs.get<Vec3>());
            break;
            case MATERIAL_PROPERTY_TYPE_VEC4:
                set(*rhs.get<Vec4>());
            break;
            case MATERIAL_PROPERTY_TYPE_MAT3:
                set(*rhs.get<Mat3>());
            break;
            case MATERIAL_PROPERTY_TYPE_MAT4:
                set(*rhs.get<Mat4>());
            break;
            case MATERIAL_PROPERTY_TYPE_TEXTURE:
                set(*rhs.get<TexturePtr>());
            break;
        }

        return *this;
    }

    PropertyValue(PropertyValue&& rhs) noexcept :
        type(std::move(rhs.type)),
        data(rhs.data) {
        rhs.data = nullptr;
    }

    PropertyValue(const PropertyValue& rhs) {
        *this = rhs;
    }

    ~PropertyValue() {
        if(data) {
            clear();
        }
    }

    void clear() override {
        if(!data) {
            return;
        }

        switch(type) {
            case MATERIAL_PROPERTY_TYPE_BOOL:
                unset<bool>();
            break;
                case MATERIAL_PROPERTY_TYPE_INT:
                unset<int32_t>();
            break;
                case MATERIAL_PROPERTY_TYPE_FLOAT:
                unset<float>();
            break;
            case MATERIAL_PROPERTY_TYPE_VEC2:
                unset<Vec2>();
            break;
            case MATERIAL_PROPERTY_TYPE_VEC3:
                unset<Vec3>();
            break;
            case MATERIAL_PROPERTY_TYPE_VEC4:
                unset<Vec4>();
            break;
            case MATERIAL_PROPERTY_TYPE_MAT3:
                unset<Mat3>();
            break;
            case MATERIAL_PROPERTY_TYPE_MAT4:
                unset<Mat4>();
            break;
            case MATERIAL_PROPERTY_TYPE_TEXTURE:
                unset<TexturePtr>();
            break;
        }

        data = nullptr;
    }

    template<typename T>
    T* get() const {
        return (T*) data;
    }

    template<typename T>
    void unset() {
        ((T*) data)->~T();
        free(data);
        data = nullptr;
    }

    bool has_value() const override {
        return bool(data);
    }
private:
    template<typename T>
    bool _set(const T& value) {
        clear();

        data = (uint8_t*) smlt::aligned_alloc(alignof(T), sizeof(T));
        assert(data);
        new (data) T(value);
        type = _impl::material_property_lookup<T>::type;

        return true;
    }

    bool do_set(const bool& value) override { return _set(value); }
    bool do_set(const int32_t& value) override { return _set(value); }
    bool do_set(const float& value) override { return _set(value); }
    bool do_set(const Vec2& value) override { return _set(value); }
    bool do_set(const Vec3& value) override { return _set(value); }
    bool do_set(const Vec4& value) override { return _set(value); }
    bool do_set(const Mat3& value) override { return _set(value); }
    bool do_set(const Mat4& value) override { return _set(value); }
    bool do_set(const TexturePtr& value) override { return _set(value); }

    void do_get(bool** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_BOOL) ? (bool*) data : nullptr; }
    void do_get(int32_t** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_INT) ? (int32_t*) data : nullptr; }
    void do_get(float** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_FLOAT) ? (float*) data : nullptr; }
    void do_get(Vec2** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_VEC2) ? (Vec2*) data : nullptr; }
    void do_get(Vec3** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_VEC3) ? (Vec3*) data : nullptr; }
    void do_get(Vec4** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_VEC4) ? (Vec4*) data : nullptr; }
    void do_get(Mat3** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_MAT3) ? (Mat3*) data : nullptr; }
    void do_get(Mat4** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_MAT4) ? (Mat4*) data : nullptr; }
    void do_get(TexturePtr** result) const override { *result = (type == MATERIAL_PROPERTY_TYPE_TEXTURE) ? (TexturePtr*) data : nullptr; }
};


}
