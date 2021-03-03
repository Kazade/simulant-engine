#pragma once

namespace smlt {

/* All materials and passes inherit the properties and
 * values of the baseline material. Overriders allow two things:
 *
 * 1. Overriding the value of the baseline material
 * 2. Adding additional property values (e.g. shader uniforms)
 */

class MaterialPropertyOverrider {
public:
    template<typename T>
    bool override_core_property(const char* name, const T& value);

    MaterialPropertyOverrider();
    MaterialPropertyOverrider(const MaterialPropertyOverrider* parent);

    template<typename T>
    void fetch_property_value(const char* name, T* out);

    bool is_core_property(const char* name) const {
        return BASELIN
    }

private:
    std::unordered_map<unsigned, Colour> colour_properties_;
    std::unordered_map<unsigned, float> float_properties_;
    std::unordered_map<unsigned, bool> bool_properties_;
};

}
