#pragma once

#include "../color.h"

namespace smlt {

struct PBRValues {
    smlt::Color base_color;
    float metallic;
    float roughness;
    smlt::Color specular_color;
    float specular;
};

struct TraditionalValues {
    smlt::Color ambient;
    smlt::Color diffuse;
    smlt::Color specular;
    float shininess;
};

PBRValues traditional_to_pbr(const smlt::Color& ambient,
                             const smlt::Color& diffuse,
                             const smlt::Color& specular, float shininess);

TraditionalValues pbr_to_traditional(const smlt::Color& base_color,
                                     float metallic, float roughness,
                                     const smlt::Color& specular_color,
                                     float specular);

} // namespace smlt
