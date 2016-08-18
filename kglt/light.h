#ifndef LIGHT_H_INCLUDED
#define LIGHT_H_INCLUDED

#include "generic/managed.h"
#include "object.h"
#include "generic/identifiable.h"
#include "types.h"

#include "utils/parent_setter_mixin.h"

#include "kazmath/vec4.h"

namespace kglt {

class Light :
    public ParentSetterMixin<MoveableObject>,
    public generic::Identifiable<LightID>,
    public BoundableEntity,
    public Managed<Light> {

public:
    typedef std::shared_ptr<Light> ptr;

    Light(LightID lid, Stage* stage);
    void set_type(LightType type) {
        type_ = type;

        // We should never cull directional lights
        culling_mode_ = (type_ == LIGHT_TYPE_DIRECTIONAL) ?
            RENDERABLE_CULLING_MODE_NEVER : RENDERABLE_CULLING_MODE_PARTITIONER;
    }

    /*
     *  Direction (ab)uses the light's position.
     *  Setting the direction implicitly sets the light type to directional
     *
     *  Direction is stored reversed in the position.
     */
    Vec3 direction() const {
        return absolute_position();
    }

    void set_direction(float x, float y, float z) {
        set_direction(Vec3(x, y, z));
    }

    void set_direction(const kmVec3& dir) {
        set_type(LIGHT_TYPE_DIRECTIONAL);
        set_absolute_position(-dir.x, -dir.y, -dir.z);
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

    /** Returns the owner stage's global ambient value. */
    kglt::Colour global_ambient() const { return stage->ambient_light(); }

    void set_attenuation(float range, float constant, float linear, float quadratic);
    void set_attenuation_from_range(float range);

    float range() const { return range_; }
    float constant_attenuation() const { return const_attenuation_; }
    float linear_attenuation() const { return linear_attenuation_; }
    float quadratic_attenuation() const { return quadratic_attenuation_; }

    /** Boundable interface **/
    const AABB transformed_aabb() const {
        AABB result;
        Vec3 abs_pos = absolute_position();
        kmAABB3Initialize(&result, &abs_pos, range(), range(), range());
        return result;
    }

    const AABB aabb() const {
        AABB result;
        kmAABB3Initialize(&result, nullptr, range(), range(), range());
        return result;
    }

    void ask_owner_for_destruction();

    unicode __unicode__() const {
        if(has_name()) {
            return name();
        } else {
            return _u("Light {0}").format(this->id());
        }
    }

    RenderableCullingMode renderable_culling_mode() const { return culling_mode_; }

private:
    LightType type_;

    kglt::Colour ambient_;
    kglt::Colour diffuse_;
    kglt::Colour specular_;

    float range_;
    float const_attenuation_;
    float linear_attenuation_;
    float quadratic_attenuation_;

    RenderableCullingMode culling_mode_ = RENDERABLE_CULLING_MODE_PARTITIONER;
};

}
#endif
