#include "pbr.h"
#include "../color.h"

namespace smlt {
TraditionalValues pbr_to_traditional(const Color& base_color, float metallic,
                                     float roughness) {
    TraditionalValues v;

    v.ambient = base_color * 0.1f;
    v.ambient.a = base_color.a;
    v.diffuse = base_color;
    v.diffuse.a = base_color.a;

    v.specular = base_color * metallic;
    v.specular.a = base_color.a;
    v.shininess = (128.0f * (1.0f - roughness));

    return v;
}

PBRValues traditional_to_pbr(const Color& ambient, const Color& diffuse,
                             const Color& specular, float shininess) {
    PBRValues pbr;
    pbr.base_color = diffuse + ambient;
    pbr.metallic = 0.0f;
    pbr.roughness = 1.0f - (shininess / 128.0f);
    return pbr;
}

} // namespace smlt
