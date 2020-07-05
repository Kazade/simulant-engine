#pragma once

#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../asset.h"
#include "../loadable.h"
#include "../types.h"


namespace smlt {

enum EmitterType {
    PARTICLE_EMITTER_POINT,
    PARTICLE_EMITTER_BOX
};

struct Emitter {
    EmitterType type = PARTICLE_EMITTER_POINT;
    Vec3 relative_position;
    Vec3 direction = Vec3::POSITIVE_Y;
    Vec3 dimensions = Vec3(100, 100, 100);

    std::pair<float, float> duration_range = std::make_pair(0.0, 0.0);
    std::pair<float, float> repeat_delay_range = std::make_pair(0.0, 0.0);
    std::pair<float, float> velocity_range = std::make_pair(1.0, 1.0);
    std::pair<float, float> ttl_range = std::make_pair(5.0, 5.0);

    Degrees angle;
    Colour colour = Colour::WHITE;

    float emission_rate = 10.0f;
};


struct Particle;
class ParticleScript;

class Manipulator {
public:
    Manipulator(ParticleScript* script, const std::string& name):
        name_(name),
        particle_script_(script) {}

    virtual ~Manipulator() {}

    void manipulate(ParticleSystem* system, Particle* particles, std::size_t particle_count, float dt) const {
        do_manipulate(system, particles, particle_count, dt);
    }

    virtual void set_linear_curve(float rate);

    virtual void set_bell_curve(float peak, float deviation);

private:
    std::string name_;
    virtual void do_manipulate(ParticleSystem* system, Particle* particles, std::size_t particle_count, float dt) const = 0;

protected:
    typedef std::function<float (float, float, float)> CurveFunc;
    ParticleScript* particle_script_;
    CurveFunc curve_;
};

typedef std::shared_ptr<Manipulator> ManipulatorPtr;

class ParticleScript:
    public Asset,
    public Loadable,
    public generic::Identifiable<ParticleScriptID>,
    public RefCounted<ParticleScript>,
    public ChainNameable<ParticleScript> {

public:
    const static int MAX_EMITTER_COUNT = 8;

    struct BuiltIns {
        static const std::string FIRE;
    };


    ParticleScript(ParticleScriptID id, AssetManager* asset_manager);

    std::size_t emitter_count() const;
    const Emitter* emitter(std::size_t i) const;

    /* Note: this is an unstable API */
    Emitter* mutable_emitter(std::size_t i);

    std::size_t manipulator_count() const;
    const Manipulator* manipulator(std::size_t i) const;

    std::size_t quota() const;

    /* FIXME: Surely this should be on the emitter? */
    float particle_width() const;
    float particle_height() const;

    bool cull_each() const;
    MaterialPtr material() const;
    bool has_repeating_emitters() const;

    void add_manipulator(std::shared_ptr<Manipulator> manipulator);
    void clear_manipulators();

    void push_emitter(const Emitter& emitter);
    void clear_emitters();

    void set_name(const std::string& name);
    void set_quota(std::size_t quota);

    void set_particle_width(float w);
    void set_particle_height(float h);
    void set_cull_each(bool v);
    void set_material(MaterialPtr material);

private:
    ParticleScript* owner_ = nullptr;

    std::string name_;
    std::size_t quota_ = 0;
    float particle_width_ = 100.0f;
    float particle_height_ = 100.0f;
    bool cull_each_ = false;

    std::array<Emitter, ParticleScript::MAX_EMITTER_COUNT> emitters_;
    uint16_t emitter_count_ = 0;

    std::vector<ManipulatorPtr> manipulators_;
    MaterialPtr material_;
};

}
