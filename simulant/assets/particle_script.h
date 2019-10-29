#pragma once

#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../asset.h"
#include "../loadable.h"
#include "../types.h"


namespace smlt {

class ParticleScriptImpl;
class ParticleScriptTransaction;

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


class Particle;
class ParticleScript;

class Manipulator {
public:
    Manipulator(ParticleScript* script, const std::string& name):
        name_(name),
        particle_script_(script) {}

    void manipulate(ParticleSystem* system, Particle* particles, std::size_t particle_count, float dt) {
        do_manipulate(system, particles, particle_count, dt);
    }

    virtual void set_linear_curve(float rate);

    virtual void set_bell_curve(float peak, float deviation);

private:
    std::string name_;
    virtual void do_manipulate(ParticleSystem* system, Particle* particles, std::size_t particle_count, float dt) = 0;

protected:
    ParticleScript* particle_script_;
    std::function<float (float, float, float)> curve_;
};

typedef std::shared_ptr<Manipulator> ManipulatorPtr;

class ParticleScript:
    public Asset,
    public AtomicAsset<ParticleScript, ParticleScriptImpl, ParticleScriptTransaction>,
    public Loadable,
    public generic::Identifiable<ParticleScriptID>,
    public RefCounted<ParticleScript> {

public:
    const static int MAX_EMITTER_COUNT = 8;

    ParticleScript(ParticleScriptID id, AssetManager* asset_manager);

    std::size_t emitter_count() const;
    const Emitter* emitter(std::size_t i) const;

    std::size_t manipulator_count() const;
    const Manipulator* manipulator(std::size_t i) const;

    std::size_t quota() const;

    /* FIXME: Surely this should be on the emitter? */
    float particle_width() const;
    float particle_height() const;

    bool cull_each() const;
    MaterialPtr material() const;
    bool has_repeating_emitters() const;
};

}
