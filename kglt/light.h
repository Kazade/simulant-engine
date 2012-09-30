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
        set_ambient(kglt::Colour(0.2f, 0.2f, 0.2f, 1.0f));
        set_diffuse(kglt::Colour(1.0f, 1.0f, 1.0f, 1.0f));
        set_specular(kglt::Colour(1.0f, 1.0f, 1.0f, 1.0f));
        set_attenuation_from_range(100.0);
    }

    void set_type(LightType type) { type_ = type; }

    void set_diffuse(const kglt::Colour& colour) {
        diffuse_ = colour;
    }

    void set_ambient(const kglt::Colour& colour) {
        ambient_ = colour;
    }

    void set_specular(const kglt::Colour& colour) {
        specular_ = colour;
    }

    LightType type() const { return type_; }
    kglt::Colour ambient() const { return ambient_; }
    kglt::Colour diffuse() const { return diffuse_; }
    kglt::Colour specular() const { return specular_; }

    void set_attenuation(float range, float constant, float linear, float quadratic);
    void set_attenuation_from_range(float range);

    kmVec3 direction() const { return direction_; }
    float range() const { return range_; }
    float constant_attenuation() const { return const_attenuation_; }
    float linear_attenuation() const { return linear_attenuation_; }
    float quadratic_attenuation() const { return quadratic_attenuation_; }

private:
    LightType type_;

    kmVec3 direction_;
    kglt::Colour ambient_;
    kglt::Colour diffuse_;
    kglt::Colour specular_;

    float range_;
    float const_attenuation_;
    float linear_attenuation_;
    float quadratic_attenuation_;

};

}
#endif
