#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <memory>

#include "particle.h"
#include "../../math/vec3.h"
#include "../../math/degrees.h"
#include "../../colour.h"

namespace smlt {

class ParticleSystem;

namespace particles {

enum EmitterType {
    PARTICLE_EMITTER_POINT,
    PARTICLE_EMITTER_BOX
};

const std::unordered_map<std::string, EmitterType> EMITTER_LOOKUP = {
    { "point", PARTICLE_EMITTER_POINT },
    { "box", PARTICLE_EMITTER_BOX }
};

class Emitter {
public:
    Emitter(ParticleSystem& system):
        system_(system) {}

    void set_type(EmitterType type) { type_ = type; }
    EmitterType type() const { return type_; }

    //Relative to the particle system
    smlt::Vec3 relative_position() const {
        return relative_position_;
    }

    void set_direction(const smlt::Vec3& dir) { direction_ = dir; }
    smlt::Vec3 direction() const {
        return direction_;
    }

    void set_angle(const Degrees& degs) { angle_ = degs; }
    Degrees angle() const {
        return angle_;
    }

    void set_colour(const smlt::Colour& col) { colour_ = col; }
    smlt::Colour colour() const {
        return colour_;
    }

    void set_emission_rate(const float rate) { emission_rate_ = rate; }
    float emission_rate() const {
        return emission_rate_;
    }

    void set_width(float w) { dimensions_.x = w; }
    void set_height(float h) { dimensions_.y = h; }
    void set_depth(float d) { dimensions_.z = d; }

    float width() const { return dimensions_.x; }
    float height() const { return dimensions_.y; }
    float depth() const { return dimensions_.z; }

    void set_ttl(float seconds);
    void set_ttl_range(float min_seconds, float max_seconds);
    std::pair<float, float> ttl_range() const;

    void set_repeat_delay(float seconds);
    void set_repeat_delay_range(float min_seconds, float max_seconds);
    std::pair<float, float> repeat_delay_range() const;

    void set_velocity(float vel);
    void set_velocity_range(float min_vel, float max_vel);
    std::pair<float, float> velocity_range() const;

    void set_duration(float seconds);
    void set_duration_range(float min_seconds, float max_seconds);
    std::pair<float, float> duration_range() const;

    std::vector<Particle> do_emit(float dt, uint32_t max_to_emit);

    ParticleSystem& system() { return system_; }

    void update(float dt);

    void activate();
    void deactivate();
    bool is_active() const { return is_active_; }

private:
    ParticleSystem& system_;
    EmitterType type_ = PARTICLE_EMITTER_POINT;
    smlt::Vec3 relative_position_ = smlt::Vec3();
    smlt::Vec3 direction_ = smlt::Vec3(1, 0, 0);
    smlt::Vec3 dimensions_ = smlt::Vec3(100, 100, 100);

    std::pair<float, float> duration_range_ = std::make_pair(0.0, 0.0);
    std::pair<float, float> repeat_delay_range_ = std::make_pair(0.0, 0.0);
    std::pair<float, float> velocity_range_ = std::make_pair(1.0, 1.0);
    std::pair<float, float> ttl_range_ = std::make_pair(5.0, 5.0);

    Degrees angle_ = Degrees(0);
    smlt::Colour colour_ = smlt::Colour::WHITE;
    float emission_rate_ = 10.0;

    float emission_accumulator_ = 0.0;

    float time_active_ = 0.0;
    float current_duration_ = 0.0;

    bool is_active_ = true;
};

typedef std::shared_ptr<Emitter> EmitterPtr;

}
}
