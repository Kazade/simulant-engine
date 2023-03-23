#include "material_property_overrider.h"
#include "../../../logging.h"

namespace smlt {

bool valid_name(const char* name) {
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

bool MaterialPropertyOverrider::check_existance(const MaterialPropertyNameHash hsh) const {
    if(is_core_property(hsh)) {
        return true;
    }

    return properties_.count(hsh) > 0;
}

bool MaterialPropertyOverrider::check_existance(const char* property_name) const {
    return check_existance(material_property_hash(property_name));
}

bool MaterialPropertyOverrider::clear_override(const unsigned hsh) {
    auto v = find_core_property(hsh);
    if(v) {
        v->clear();
        return true;
    }

    auto it = properties_.find(hsh);
    if(it != properties_.end()) {
        properties_.erase(it);
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
        auto it = properties_.find(hsh);
        if(it != properties_.end()) {
            *type = it->second.type;
        } else {
            return false;
        }
    }

    return true;
}
}
