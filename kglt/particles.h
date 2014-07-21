#ifndef PARTICLES_H
#define PARTICLES_H

#include <memory>
#include <unordered_map>

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/protected_ptr.h"

#include "utils/parent_setter_mixin.h"

#include "sound.h"
#include "object.h"
#include "interfaces.h"
#include "types.h"

namespace kglt {

enum ParticleEmitterType {
    PARTICLE_EMITTER_POINT,
    PARTICLE_EMITTER_BOX
};

const std::unordered_map<unicode, ParticleEmitterType> EMITTER_LOOKUP = {
    { "point", PARTICLE_EMITTER_POINT },
    { "box", PARTICLE_EMITTER_BOX }
};

class ParticleEmitter {
public:
    void set_type(ParticleEmitterType type) { type_ = type; }
    ParticleEmitterType type() const { return type_; }

    //Relative to the particle system
    kglt::Vec3 relative_position() const {
        return relative_position_;
    }

    void set_direction(const kglt::Vec3& dir) { direction_ = dir; }
    kglt::Vec3 direction() const {
        return direction_;
    }

    void set_velocity(float vel) { velocity_ = vel; }
    float velocity() const {
        return velocity_;
    }

    void set_ttl(float ttl) { ttl_ = ttl; }
    float ttl() const {
        return ttl_;
    }

    void set_angle(const Degrees& degs) { angle_ = degs; }
    Degrees angle() const {
        return angle_;
    }

    void set_colour(const kglt::Colour& col) { colour_ = col; }
    kglt::Colour colour() const {
        return colour_;
    }

    void set_emission_rate(const int rate) { emission_rate_ = rate; }
    int emission_rate() const {
        return emission_rate_;
    }

private:
    ParticleEmitterType type_ = PARTICLE_EMITTER_POINT;
    kglt::Vec3 relative_position_ = kglt::Vec3();
    kglt::Vec3 direction_ = kglt::Vec3(1, 0, 0);
    float velocity_ = 1.0;
    float ttl_ = 5.0;
    Degrees angle_ = Degrees(0);
    kglt::Colour colour_ = kglt::Colour::WHITE;
    int emission_rate_ = 10;
};

typedef std::shared_ptr<ParticleEmitter> EmitterPtr;

class ParticleSystem :
    public virtual BoundableEntity,
    public Nameable,
    public Managed<ParticleSystem>,
    public generic::Identifiable<ParticleSystemID>,
    public ParentSetterMixin<Object>,
    public Source,
    public Protectable,
    public Loadable {

public:
    ParticleSystem(Stage* stage, ParticleSystemID id);

    void set_name(const unicode& name) { name_ = name; }
    const bool has_name() const { return !name_.empty(); }
    const unicode name() const { return name_; }

    void set_quota(int quota) { quota_ = quota; }
    int32_t quota() const { return quota_; }

    void set_particle_width(float width) { particle_width_ = width; }
    float particle_width() const { return particle_width_; }

    void set_particle_height(float height) { particle_height_ = height; }
    float particle_height() const { return particle_height_; }

    void set_cull_each(bool val=true) { cull_each_ = val; }
    bool cull_each() const { return cull_each_; }

    int32_t emitter_count() const { return emitters_.size(); }
    EmitterPtr emitter(int32_t i) { return emitters_.at(i); }
    EmitterPtr push_emitter();
    void pop_emitter();

    //Boundable entity things
    const AABB aabb() const;
    const AABB transformed_aabb() const;

    unicode __unicode__() const { return name_; }

    void ask_owner_for_destruction();
private:
    unicode name_;
    int quota_ = 10;
    float particle_width_ = 100.0;
    float particle_height_ = 100.0;
    bool cull_each_ = false;

    std::vector<EmitterPtr> emitters_;
};

}

#endif // PARTICLES_H
