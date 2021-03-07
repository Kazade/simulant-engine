#include "material_property_overrider.h"
#include "../../../logging.h"

namespace smlt {

void MaterialPropertyOverrider::override_property_value(const char* name, const bool& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    bool_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_BOOL;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_BOOL);
}

void MaterialPropertyOverrider::override_property_value(const char* name, const float& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    float_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_FLOAT;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_FLOAT);
}

void MaterialPropertyOverrider::override_property_value(const char* name, const int32_t& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    int_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_INT;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_INT);
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Vec4& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    vec4_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC4;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_VEC4);
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Vec3& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    vec3_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC3;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_VEC3);
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Vec2& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    vec2_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC2;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_VEC2);
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Mat3& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    mat3_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_MAT3;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_MAT3);
}

void MaterialPropertyOverrider::override_property_value(const char* name, const Mat4& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    mat4_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_MAT4;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_MAT4);
}

void MaterialPropertyOverrider::override_property_value(const char* name, const TexturePtr& value) {
    if(!check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = const_hash(name);
    clear_override(hsh);
    texture_properties_[hsh] = value;
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_TEXTURE;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_TEXTURE);
}

template<typename T, typename Map>
bool fetcher(const MaterialPropertyOverrider* _this, const MaterialPropertyOverrider* parent, Map map, MaterialPropertyNameHash hsh, const T*& out) {
    auto& lookup = _this->*map;
    auto it = lookup.find(hsh);
    if(it != lookup.end()) {
        out = &it->second;
        return true;
    } else if(parent) {
        return fetcher(parent, nullptr, map, hsh, out);
    } else if(is_core_property(hsh)) {
        return core_material_property_value(hsh, out);
    } else {
        return false;
    }
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const bool*& out) const {
    auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::bool_properties_, hsh, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const float*& out) const {
    auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::float_properties_, hsh, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const int32_t*& out) const {
    const auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::int_properties_, hsh, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const Colour*& out) const {
    /* FIXME? Risky cast from Colour -> Vec4.. should be OK? */
    const auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::vec4_properties_, hsh, (const Vec4*&) out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const Vec2*& out) const {
    const auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::vec2_properties_, hsh, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const Vec3*& out) const {
    const auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::vec3_properties_, hsh, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const Vec4*& out) const {
    const auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::vec4_properties_, hsh, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const Mat3*& out) const {
    const auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::mat3_properties_, hsh, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const Mat4*& out) const {
    const auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::mat4_properties_, hsh, out);
}

bool MaterialPropertyOverrider::fetch_property_value(const char* name, const TexturePtr*& out) const {
    const auto hsh = const_hash(name);
    return fetcher(this, parent_, &MaterialPropertyOverrider::texture_properties_, hsh, out);
}

bool MaterialPropertyOverrider::check_existance(const char* property_name) const {
    if(is_core_property(property_name)) {
        return true;
    }

    if(parent_) {
        return parent_->check_existance(property_name);
    } else {
        return all_overrides_.count(const_hash(property_name)) > 0;
    }
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
            case MATERIAL_PROPERTY_TYPE_TEXTURE:
                texture_properties_.erase(hsh);
            break;
            default:
            return false;
        }

        all_overrides_.erase(hsh);
        return true;
    }

    return false;
}

bool MaterialPropertyOverrider::property_type(const char* property_name, MaterialPropertyType* type) const {
    auto hsh = const_hash(property_name);

    if(parent_) {
        return parent_->property_type(property_name, type);
    }

    if(is_core_property(hsh)) {
        return core_property_type(hsh, type);
    } else {
        auto it = all_overrides_.find(hsh);
        if(it != all_overrides_.end()) {
            *type = it->second;
        } else {
            return false;
        }
    }

    return true;
}
}
