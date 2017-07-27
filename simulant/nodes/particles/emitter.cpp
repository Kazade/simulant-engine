#include "emitter.h"
#include "../../stage.h"
#include "../particle_system.h"
#include "../../utils/random.h"

namespace smlt {
namespace particles {

void Emitter::activate() {
    is_active_ = true;
    time_active_ = 0.0;
}

void Emitter::deactivate() {
    is_active_ = false;
}

void Emitter::update(float dt) {
    time_active_ += dt;

    if(current_duration_ && time_active_ >= current_duration_) {
        deactivate();

        float repeat_delay = random_gen::random_float(repeat_delay_range_.first, repeat_delay_range_.second);
        if(repeat_delay > 0) {
            system().stage->window->idle->add_timeout_once(repeat_delay, std::bind(&Emitter::activate, this));
        }
    }
}

std::vector<Particle> Emitter::do_emit(float dt, uint32_t max) {
    std::vector<Particle> new_particles;

    if(!max) {
        return new_particles; //Do nothing
    }

    emission_accumulator_ += dt; //Buffer time

    float decrement = 1.0 / float(emission_rate()); //Work out how often to emit per second

    uint32_t to_emit = max;
    while(emission_accumulator_ > decrement) {
        //EMIT THE PARTICLE!
        Particle p;
        if(type() == PARTICLE_EMITTER_POINT) {
            p.position = system().absolute_position() + relative_position();
        } else {
            p.position = system().absolute_position() + relative_position();

            float hw = dimensions_.x * 0.5;
            float hh = dimensions_.y * 0.5;
            float hd = dimensions_.z * 0.5;

            p.position.x += random_gen::random_float(-hw, hw);
            p.position.y += random_gen::random_float(-hh, hh);
            p.position.z += random_gen::random_float(-hd, hd);
        }

        Vec3 dir = direction();
        if(angle().value != 0) {
            Radians ang(angle()); //Convert from degress to radians
            ang.value *= random_gen::random_float(0, 1); //Multiply by a random unit float
            dir = dir.random_deviant(ang).normalized();
        }

        p.velocity = dir * random_gen::random_float(velocity_range().first, velocity_range().second);

        //We have to rotate the velocity by the system, because if the particle system is attached to something (e.g. the back of a spaceship)
        //when that entity rotates we want the velocity to stay pointing relative to the entity
        auto rot = system().absolute_rotation();

        p.velocity *= rot;

        p.ttl = random_gen::random_float(ttl_range().first, ttl_range().second);
        p.colour = colour();
        p.dimensions = smlt::Vec2(system().particle_width(), system().particle_height());

        //FIXME: Initialize other properties
        new_particles.push_back(p);

        emission_accumulator_ -= decrement; //Decrement the accumulator while we can
        to_emit--;
        if(!to_emit) {
            break;
        }
    }

    return new_particles;
}


void Emitter::set_ttl(float seconds) {
    ttl_range_ = std::make_pair(seconds, seconds);
}

void Emitter::set_ttl_range(float min_seconds, float max_seconds) {
    if(min_seconds > max_seconds) {
        throw std::logic_error("min_seconds can't be greater than max_seconds");
    }

    ttl_range_ = std::make_pair(min_seconds, max_seconds);
}

std::pair<float, float> Emitter::ttl_range() const {
    return ttl_range_;
}

void Emitter::set_repeat_delay(float seconds) {
    set_repeat_delay_range(seconds, seconds);
}

void Emitter::set_repeat_delay_range(float min_seconds, float max_seconds) {
    repeat_delay_range_ = std::make_pair(min_seconds, max_seconds);
}

std::pair<float, float> Emitter::repeat_delay_range() const {
    return repeat_delay_range_;
}

void Emitter::set_velocity(float vel) {
    set_velocity_range(vel, vel);
}

void Emitter::set_velocity_range(float min_vel, float max_vel) {
    velocity_range_ = std::make_pair(min_vel, max_vel);
}

std::pair<float, float> Emitter::velocity_range() const {
    return velocity_range_;
}

void Emitter::set_duration(float seconds) {
    set_duration_range(seconds, seconds);
}

void Emitter::set_duration_range(float min_seconds, float max_seconds) {
    duration_range_ = std::make_pair(min_seconds, max_seconds);
    current_duration_ = random_gen::random_float(duration_range_.first, duration_range_.second);
}

std::pair<float, float> Emitter::duration_range() const {
    return duration_range_;
}

}
}
