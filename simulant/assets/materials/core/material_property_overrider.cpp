#include "material_property_overrider.h"

namespace smlt {

void MaterialPropertyOverrider::override_property_value(const char* name, const bool& value) {
    auto hsh = const_hash(name);
    clear_override(hsh);
    bool_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_BOOL;
}

void MaterialPropertyOverrider::override_property_value(const char* name, const float& value) {
    auto hsh = const_hash(name);
    clear_override(hsh);
    float_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_FLOAT;
}

void MaterialPropertyOverrider::override_property_value(const char* name, const int32_t& value) {
    auto hsh = const_hash(name);
    clear_override(hsh);
    int_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_INT;
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Vec4& value) {
    auto hsh = const_hash(name);
    clear_override(hsh);
    vec4_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC4;
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Vec3& value) {
    auto hsh = const_hash(name);
    clear_override(hsh);
    vec3_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC3;
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Vec2& value) {
    auto hsh = const_hash(name);
    clear_override(hsh);
    vec2_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC2;
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Mat3& value) {
    auto hsh = const_hash(name);
    clear_override(hsh);
    mat3_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_MAT3;
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Mat4& value) {
    auto hsh = const_hash(name);
    clear_override(hsh);
    mat4_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_MAT4;
}

template<typename T>
bool fetcher(MaterialPropertyOverrider* parent, std::unordered_map<unsigned, T>& lookup, const char* name, T* out) {
    if(parent) {
        return fetcher(nullptr, lookup, name, out);
    }

    auto it = lookup.find(const_hash(name));
    if(it != lookup.end()) {
        *out = it->second;
        return true;
    } else if(is_core_property(name)) { // FIXME: calc hash once
        return core_material_property_value(name, out);
    } else {
        return false;
    }
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, bool* out) {
    return fetcher(parent_, bool_properties_, name, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, float* out) {
    return fetcher(parent_, float_properties_, name, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, int32_t* out) {
    return fetcher(parent_, int_properties_, name, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, Colour* out) {
    /* FIXME? Risky cast from Colour -> Vec4.. should be OK? */
    return fetcher(parent_, vec4_properties_, name, (Vec4*) out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, Vec2* out) {
    return fetcher(parent_, vec2_properties_, name, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, Vec3* out) {
    return fetcher(parent_, vec3_properties_, name, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, Vec4* out) {
    return fetcher(parent_, vec4_properties_, name, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, Mat3* out) {
    return fetcher(parent_, mat3_properties_, name, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, Mat4* out) {
    return fetcher(parent_, mat4_properties_, name, out);
}

bool MaterialPropertyOverrider::clear_override(const unsigned hsh) {
    auto it = all_overrides_.find(hsh);
    if(it != all_overrides_.end()) {
        auto type = it->second;
        switch(type) {
            case MATERIAL_PROPERTY_TYPE_BOOL:
                bool_properties_.erase(hsh);
            break;
            case MATERIAL_PROPERTY_TYPE_FLOAT:
                float_properties_.erase(hsh);
            break;
            case MATERIAL_PROPERTY_TYPE_INT:
                int_properties_.erase(hsh);
            break;
            case MATERIAL_PROPERTY_TYPE_VEC2:
                vec2_properties_.erase(hsh);
            break;
            case MATERIAL_PROPERTY_TYPE_VEC3:
                vec3_properties_.erase(hsh);
            break;
            case MATERIAL_PROPERTY_TYPE_VEC4:
                vec4_properties_.erase(hsh);
            break;
            case MATERIAL_PROPERTY_TYPE_MAT3:
                mat3_properties_.erase(hsh);
            break;
            case MATERIAL_PROPERTY_TYPE_MAT4:
                mat4_properties_.erase(hsh);
            break;
            default:
            return false;
        }
        all_overrides_.erase(hsh);
        return true;
    }

    return false;
}
}
