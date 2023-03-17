#include "material_property_overrider.h"
#include "../../../logging.h"

namespace smlt {

static inline bool valid_name(const char* name) {
    const char* i = name;
    while(*i) {
        const char c = *i++;
        if(c >= 'A' && c <= 'Z') continue;
        if(c >= 'a' && c <= 'z') continue;
        if(c >= '0' && c <= '9') continue;
        if(c == '_' || c == '[' || c == ']') continue;

        S_DEBUG("Invalid char: {0}", c);
        return false;
    }

    return true;
}

void MaterialPropertyOverrider::set_property_value(const char* name, const bool& value) {
    if(!valid_name(name)) {
        S_WARN("Ignoring invalid property name: {0}", name);
        return;
    }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    bool_properties_.push_back(PropertyValue<bool>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_BOOL;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_BOOL);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const float& value) {
    if(!valid_name(name)) {
        S_WARN("Ignoring invalid property name: {0}", name);
        return;
    }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    float_properties_.push_back(PropertyValue<float>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_FLOAT;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_FLOAT);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const int32_t& value) {
    if(!valid_name(name)) {
        S_WARN("Ignoring invalid property name: {0}", name);
        return;
    }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    int_properties_.push_back(PropertyValue<int32_t>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_INT;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_INT);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const Colour& value) {
    set_property_value(name, (const Vec4&) value);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const Vec4& value) {
    if(!valid_name(name)) {
        S_WARN("Ignoring invalid property name: {0}", name);
        return;
    }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    vec4_properties_.push_back(PropertyValue<Vec4>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC4;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_VEC4);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const Vec3& value) {
    if(!valid_name(name)) {
        S_WARN("Ignoring invalid property name: {0}", name);
        return;
    }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    vec3_properties_.push_back(PropertyValue<Vec3>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC3;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_VEC3);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const Vec2& value) {
    if(!valid_name(name)) {
        S_WARN("Ignoring invalid property name: {0}", name);
        return;
    }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    vec2_properties_.push_back(PropertyValue<Vec2>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_VEC2;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_VEC2);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const Mat3& value) {
    if(!valid_name(name)) { S_WARN("Ignoring invalid property name: {0}", name); return; }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    mat3_properties_.push_back(PropertyValue<Mat3>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_MAT3;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_MAT3);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const Mat4& value) {
    if(!valid_name(name)) {
        S_WARN("Ignoring invalid property name: {0}", name);
        return;
    }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    mat4_properties_.push_back(PropertyValue<Mat4>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_MAT4;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_MAT4);
}

void MaterialPropertyOverrider::set_property_value(const char* name, const TexturePtr& value) {
    if(!valid_name(name)) {
        S_WARN("Ignoring invalid property name: {0}", name);
        return;
    }

    if(parent_ && !parent_->check_existance(name)) {
        S_WARN("Ignoring unknown property override for {0}", name);
        return;
    }

    auto hsh = material_property_hash(name);
    clear_override(hsh);
    texture_properties_.push_back(PropertyValue<TexturePtr>(hsh, value));
    all_overrides_[hsh] = MATERIAL_PROPERTY_TYPE_TEXTURE;

    on_override(hsh, name, MATERIAL_PROPERTY_TYPE_TEXTURE);
}

template<typename T, typename Map>
static inline bool fetcher(const MaterialPropertyOverrider* const __restrict__ _this, const MaterialPropertyOverrider* const __restrict__ parent, const Map& map, MaterialPropertyNameHash hsh, const T*& out) {
    auto& lookup = _this->*map;

    auto s = lookup.size();
    if(s) {
        auto v = &lookup[0];
        for(std::size_t i = 0; i < s; ++i, ++v) {
            if(v->hsh == hsh) {
                out = &v->value;
                return true;
            }
        }
    }

    if(parent) {
        return fetcher(parent, nullptr, map, hsh, out);
    } else if(is_core_property(hsh)) {
        return core_material_property_value(hsh, out);
    } else {
        return false;
    }
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const bool*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::bool_properties_, hsh, out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const float*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::float_properties_, hsh, out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const int32_t*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::int_properties_, hsh, out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const Colour*& out) const {
    /* FIXME? Risky cast from Colour -> Vec4.. should be OK? */
    return fetcher(this, parent_, &MaterialPropertyOverrider::vec4_properties_, hsh, (const Vec4*&) out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const Vec2*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::vec2_properties_, hsh, out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const Vec3*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::vec3_properties_, hsh, out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const Vec4*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::vec4_properties_, hsh, out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const Mat3*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::mat3_properties_, hsh, out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const Mat4*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::mat4_properties_, hsh, out);
}

bool MaterialPropertyOverrider::property_value(const MaterialPropertyNameHash hsh, const TexturePtr*& out) const {
    return fetcher(this, parent_, &MaterialPropertyOverrider::texture_properties_, hsh, out);
}

bool MaterialPropertyOverrider::check_existance(const MaterialPropertyNameHash hsh) const {
    if(is_core_property(hsh)) {
        return true;
    }

    return all_overrides_.count(hsh) > 0;
}

bool MaterialPropertyOverrider::check_existance(const char* property_name) const {
    return check_existance(material_property_hash(property_name));
}

bool MaterialPropertyOverrider::clear_override(const unsigned hsh) {
    auto it = all_overrides_.find(hsh);
    if(it != all_overrides_.end()) {
        auto type = it->second;
        switch(type) {
            case MATERIAL_PROPERTY_TYPE_BOOL:
                bool_properties_.erase(
                    std::remove_if(
                        bool_properties_.begin(),
                        bool_properties_.end(),
                        [&](const PropertyValue<bool>& e) -> bool { return e.hsh == hsh; }
                    ),
                    bool_properties_.end()
                );
            break;
            case MATERIAL_PROPERTY_TYPE_FLOAT:
                float_properties_.erase(
                    std::remove_if(
                        float_properties_.begin(),
                        float_properties_.end(),
                        [&](const PropertyValue<float>& e) -> bool { return e.hsh == hsh; }
                    ),
                    float_properties_.end()
                );
            break;
            case MATERIAL_PROPERTY_TYPE_INT:
            int_properties_.erase(
                std::remove_if(
                    int_properties_.begin(),
                    int_properties_.end(),
                    [&](const PropertyValue<int32_t>& e) -> bool { return e.hsh == hsh; }
                ),
                int_properties_.end()
            );
            break;
            case MATERIAL_PROPERTY_TYPE_VEC2:
            vec2_properties_.erase(
                std::remove_if(
                    vec2_properties_.begin(),
                    vec2_properties_.end(),
                    [&](const PropertyValue<Vec2>& e) -> bool { return e.hsh == hsh; }
                ),
                vec2_properties_.end()
            );
            break;
            case MATERIAL_PROPERTY_TYPE_VEC3:
            vec3_properties_.erase(
                std::remove_if(
                    vec3_properties_.begin(),
                    vec3_properties_.end(),
                    [&](const PropertyValue<Vec3>& e) -> bool { return e.hsh == hsh; }
                ),
                vec3_properties_.end()
            );
            break;
            case MATERIAL_PROPERTY_TYPE_VEC4:
            vec4_properties_.erase(
                std::remove_if(
                    vec4_properties_.begin(),
                    vec4_properties_.end(),
                    [&](const PropertyValue<Vec4>& e) -> bool { return e.hsh == hsh; }
                ),
                vec4_properties_.end()
            );
            break;
            case MATERIAL_PROPERTY_TYPE_MAT3:
            mat3_properties_.erase(
                std::remove_if(
                    mat3_properties_.begin(),
                    mat3_properties_.end(),
                    [&](const PropertyValue<Mat3>& e) -> bool { return e.hsh == hsh; }
                ),
                mat3_properties_.end()
            );
            break;
            case MATERIAL_PROPERTY_TYPE_MAT4:
            mat4_properties_.erase(
                std::remove_if(
                    mat4_properties_.begin(),
                    mat4_properties_.end(),
                    [&](const PropertyValue<Mat4>& e) -> bool { return e.hsh == hsh; }
                ),
                mat4_properties_.end()
            );
            break;
            case MATERIAL_PROPERTY_TYPE_TEXTURE:
            texture_properties_.erase(
                std::remove_if(
                    texture_properties_.begin(),
                    texture_properties_.end(),
                    [&](const PropertyValue<TexturePtr>& e) -> bool { return e.hsh == hsh; }
                ),
                texture_properties_.end()
            );
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
    auto hsh = material_property_hash(property_name);

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
