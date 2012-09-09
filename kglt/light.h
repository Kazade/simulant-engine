#ifndef LIGHT_H_INCLUDED
#define LIGHT_H_INCLUDED

#include "object.h"
#include "generic/identifiable.h"
#include "types.h"

#include "kazmath/vec4.h"

namespace kglt {

enum LightType {
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_SPOT_LIGHT
};

class Light :
    public Object,
    public generic::Identifiable<LightID> {

public:
    typedef std::tr1::shared_ptr<Light> ptr;

    Light(Scene* scene, LightID lid):
        Object(scene),
        generic::Identifiable<LightID>(lid),
        type_(LIGHT_TYPE_POINT),
        range_(100.0) {

        kmVec3Fill(&direction_, 0.0, 0.0, 0.0);
        kmVec4Fill(&ambient_, 0.2f, 0.2f, 0.2f, 0.2f);
        kmVec4Fill(&diffuse_, 1.0f, 1.0f, 1.0f, 1.0f);
        kmVec4Fill(&specular_, 1.0f, 1.0f, 1.0f, 1.0f);

        set_attenuation(100.0, 1.0, 0.045, 0.0075);
    }

    void set_type(LightType type) { type_ = type; }

    LightType type() const { return type_; }
    kmVec4 ambient() const { return ambient_; }
    kmVec4 diffuse() const { return diffuse_; }
    kmVec4 specular() const { return specular_; }

    void set_attenuation(float range, float constant, float linear, float quadratic);

    kmVec3 direction() const { return direction_; }
    float range() const { return range_; }
    float constant_attenuation() const { return const_attenuation_; }
    float linear_attenuation() const { return linear_attenuation_; }
    float quadratic_attenuation() const { return quadratic_attenuation_; }

private:
    LightType type_;

    kmVec3 direction_;
    kmVec4 ambient_;
    kmVec4 diffuse_;
    kmVec4 specular_;

    float range_;
    float const_attenuation_;
    float linear_attenuation_;
    float quadratic_attenuation_;

};

}
#endif
