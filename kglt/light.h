#ifndef LIGHT_H_INCLUDED
#define LIGHT_H_INCLUDED

#include "object.h"
#include "generic/identifiable.h"
#include "types.h"
#include "boundable.h"

#include "kazmath/vec4.h"

namespace kglt {

class Light :
    public Object,
    public generic::Identifiable<LightID>,
    public Boundable {

public:
    typedef std::shared_ptr<Light> ptr;

    Light(Stage* stage, LightID lid);
    void set_type(LightType type) { type_ = type; }

    /*
     *  Direction (ab)uses the light's position.
     *  Setting the direction implicitly sets the light type to directional
     *
     *  Direction is stored reversed in the position.
     */
    kmVec3 direction() const {
        kmVec3 result;
        kmVec3Fill(&result, position().x, position().y, position().z);
        return result;
    }

    void set_direction(float x, float y, float z) {
        set_direction(Vec3(x, y, z));
    }

    void set_direction(const kmVec3& dir) {
        set_type(LIGHT_TYPE_DIRECTIONAL);
        move_to(-dir.x, -dir.y, -dir.z);
    }

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

    float range() const { return range_; }
    float constant_attenuation() const { return const_attenuation_; }
    float linear_attenuation() const { return linear_attenuation_; }
    float quadratic_attenuation() const { return quadratic_attenuation_; }

    /** Boundable interface **/
    const kmAABB absolute_bounds() const {
        kmAABB result;
        kmVec3 abs_pos = absolute_position();
        kmAABBInitialize(&result, &abs_pos, range(), range(), range());
        return result;
    }

    const kmAABB local_bounds() const {
        kmAABB result;
        kmAABBInitialize(&result, nullptr, range(), range(), range());
        return result;
    }

    const kmVec3 centre() const {
        return position();
    }

    void destroy();

private:
    LightType type_;

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
