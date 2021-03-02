#pragma once

namespace smlt {

class MaterialPropertyOverrider {
public:
    template<typename T>
    bool override_core_property(const char* name, const T& value);

    MaterialPropertyOverrider();
    MaterialPropertyOverrider(const MaterialPropertyOverrider* parent);

    template<typename T>
    void fetch_property_value(const char* name, T* out);



private:
    std::unordered_map<unsigned, Colour> colour_properties_;
    std::unordered_map<unsigned, float> float_properties_;
    std::unordered_map<unsigned, bool> bool_properties_;
};

}
