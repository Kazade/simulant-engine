//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "particles.h"

#include "../utils/random.h"
#include "../stage.h"
#include "../hardware_buffer.h"

namespace smlt {

const static VertexSpecification PS_VERTEX_SPEC(
        smlt::VERTEX_ATTRIBUTE_3F, // Position
        smlt::VERTEX_ATTRIBUTE_NONE,
        smlt::VERTEX_ATTRIBUTE_2F, // Texcoord 0
        smlt::VERTEX_ATTRIBUTE_NONE,
        smlt::VERTEX_ATTRIBUTE_NONE,
        smlt::VERTEX_ATTRIBUTE_NONE,
        smlt::VERTEX_ATTRIBUTE_NONE,
        smlt::VERTEX_ATTRIBUTE_NONE,
        smlt::VERTEX_ATTRIBUTE_NONE,
        smlt::VERTEX_ATTRIBUTE_NONE,
        smlt::VERTEX_ATTRIBUTE_4F // Diffuse
);

ParticleSystem::ParticleSystem(ParticleSystemID id, Stage* stage, SoundDriver* sound_driver):
    StageNode(stage),
    generic::Identifiable<ParticleSystemID>(id),
    Source(stage, sound_driver),       
    vertex_data_(new VertexData(PS_VERTEX_SPEC)),
    index_data_(new IndexData(INDEX_TYPE_16_BIT)) {

    set_quota(quota_); // Force hardware buffer initialization
    set_material_id(stage->assets->clone_default_material());
}

ParticleSystem::~ParticleSystem() {
    delete vertex_data_;
    vertex_data_ = nullptr;

    delete index_data_;
    index_data_ = nullptr;
}

void ParticleSystem::set_material_id(MaterialID mat_id) {
    if(!mat_id) {
        throw std::logic_error("A particle system must always have a valid material");
    }

    auto old = material_id_;

    material_id_ = mat_id;

    //Hold a reference to the material so that it's destroyed when we are
    material_ref_ = stage->assets->material(material_id_);

    signal_material_changed_(this, old, mat_id);
}

EmitterPtr ParticleSystem::push_emitter() {
    auto new_emitter = std::make_shared<ParticleEmitter>(*this);
    emitters_.push_back(new_emitter);
    calc_aabb();
    return new_emitter;
}

void ParticleSystem::pop_emitter() {
    emitters_.pop_back();
    calc_aabb();
}

void ParticleSystem::calc_aabb() {
    if(emitters_.empty()) {
        aabb_ = AABB();
        return;
    }

    AABB& result = aabb_;
    bool first = true;
    for(auto emitter: emitters_) {
        auto pos = emitter->relative_position();

        if(emitter->type() == PARTICLE_EMITTER_POINT) {
            if(pos.x > result.max().x || first) result.set_max_x(pos.x);
            if(pos.y > result.max().y || first) result.set_max_y(pos.y);
            if(pos.z > result.max().z || first) result.set_max_z(pos.z);

            if(pos.x < result.min().x || first) result.set_min_x(pos.x);
            if(pos.y < result.min().x || first) result.set_min_y(pos.y);
            if(pos.z < result.min().x || first) result.set_min_z(pos.z);
        } else {
            // If this is not a point emitter, then calculate the max/min possible for
            // each emitter using their dimensions.

            float hw = (emitter->width() / 2.0);
            float hh = (emitter->height() / 2.0);
            float hd = (emitter->depth() / 2.0);

            float minx = pos.x - hw;
            float maxx = pos.x + hw;
            float miny = pos.y - hh;
            float maxy = pos.y + hh;
            float minz = pos.z - hd;
            float maxz = pos.z + hd;

            if(maxx > result.max().x || first) result.set_max_x(maxx);
            if(maxy > result.max().y || first) result.set_max_y(maxy);
            if(maxz > result.max().z || first) result.set_max_z(maxz);

            if(minx > result.min().x || first) result.set_min_x(minx);
            if(miny > result.min().y || first) result.set_min_y(miny);
            if(minz > result.min().z || first) result.set_min_z(minz);
        }

        first = false;
    }

}

//Boundable entity things
const AABB &ParticleSystem::aabb() const {
    return aabb_;
}

void ParticleSystem::prepare_buffers() {

    // Only resize the hardware buffers if someone called set_quota()
    if(resize_buffers_) {
        auto* renderer = stage->window->renderer.get();

        if(!vertex_buffer_) {
            vertex_buffer_ = renderer->hardware_buffers->allocate(
                vertex_data_->stride() * quota_ * 4,
                HARDWARE_BUFFER_VERTEX_ATTRIBUTES,
                SHADOW_BUFFER_DISABLED
            );
        } else {
            vertex_buffer_->resize(vertex_data_->stride() * quota_ * 4);
        }

        if(!index_buffer_) {
            index_buffer_ = renderer->hardware_buffers->allocate(
                sizeof(Index) * quota_ * 6,
                HARDWARE_BUFFER_VERTEX_ARRAY_INDICES,
                SHADOW_BUFFER_DISABLED
            );
        } else {
            index_buffer_->resize(sizeof(Index) * quota_ * 6);
        }

        resize_buffers_ = false;
    }

    if(vertex_buffer_dirty_) {
        vertex_buffer_->upload(*vertex_data_);
        vertex_buffer_dirty_ = false;
    }

    if(index_buffer_dirty_) {
        index_buffer_->upload(*index_data_);
        index_buffer_dirty_ = false;
    }
}


bool ParticleSystem::has_repeating_emitters() const {
    for(auto e: emitters_) {
        auto range = e->repeat_delay_range();
        if(range.first || range.second) {
            return true;
        }
    }

    return false;
}

bool ParticleSystem::has_active_emitters() const {
    for(auto e: emitters_) {
        if(e->is_active()) {
            return true;
        }
    }

    return false;
}

void ParticleSystem::ask_owner_for_destruction() {
    stage->delete_particle_system(id());
}

void ParticleEmitter::activate() {
    is_active_ = true;
    time_active_ = 0.0;
}

void ParticleEmitter::deactivate() {
    is_active_ = false;
}

void ParticleEmitter::update(float dt) {
    time_active_ += dt;

    if(current_duration_ && time_active_ >= current_duration_) {
        deactivate();

        float repeat_delay = random_gen::random_float(repeat_delay_range_.first, repeat_delay_range_.second);
        if(repeat_delay > 0) {
            system().stage->window->idle->add_timeout_once(repeat_delay, std::bind(&ParticleEmitter::activate, this));
        }
    }
}

void ParticleSystem::set_quota(std::size_t quota) {
    if(quota == particles_.size()) {
        return;
    }

    // if the quota changed, then the hardware buffers will need resizing
    resize_buffers_ = true;

    quota_ = quota;
    if(particles_.size() > quota) {
        particles_.resize(quota);
    }
    vertex_buffer_dirty_ = index_buffer_dirty_ = true;
}

void ParticleSystem::update(float dt) {
    update_source(dt); //Update any sounds attached to this particle system

    auto original_particle_count = particles_.size();

    // Update existing particles, erase any that are dead
    for(auto it = particles_.begin(); it != particles_.end(); ) {
        Particle& particle = (*it);

        particle.position += particle.velocity * dt;
        particle.ttl -= dt;

        if(particle.ttl <= 0.0) {
            it = particles_.erase(it);
        } else {
            ++it;
        }
    }

    for(auto emitter: emitters_) {
        emitter->update(dt);

        if(!emitter->is_active()) {
            continue;
        }

        // Don't create new particles if we can't
        if(particles_.size() >= quota_) {
            continue;
        }

        auto max_can_emit = quota_ - particles_.size();
        auto new_particles = emitter->do_emit(dt, max_can_emit);
        particles_.insert(particles_.end(), new_particles.begin(), new_particles.end());
    }

    if(particles_.empty() && !has_repeating_emitters() && !has_active_emitters()) {
        // If the particles are gone, and we don't have repeating emitters and all the emitters are inactive
        // Then destroy the particle system if that's what we've been told to do
        if(destroy_on_completion()) {
            ask_owner_for_destruction();
            // No point doing anything else!
            return;
        }
    }

    auto hh = (particle_height_ * Vec3::POSITIVE_Y) * 0.5f;
    auto hw = (particle_width_ * Vec3::POSITIVE_X) * 0.5f;

    vertex_data_->move_to_start();
    vertex_data_->resize(particles_.size() * 4);
    for(auto& particle: particles_) {
        auto v1 = particle.position + (-hh + -hw);
        auto v2 = particle.position + (-hh +  hw);
        auto v3 = particle.position + ( hh +  hw);
        auto v4 = particle.position + ( hh + -hw);

        vertex_data_->position(v1);
        vertex_data_->diffuse(particle.colour);
        vertex_data_->tex_coord0(0, 0);
        vertex_data_->move_next();

        vertex_data_->position(v2);
        vertex_data_->diffuse(particle.colour);
        vertex_data_->tex_coord0(1, 0);
        vertex_data_->move_next();

        vertex_data_->position(v3);
        vertex_data_->diffuse(particle.colour);
        vertex_data_->tex_coord0(1, 1);
        vertex_data_->move_next();

        vertex_data_->position(v4);
        vertex_data_->diffuse(particle.colour);
        vertex_data_->tex_coord0(0, 1);
        vertex_data_->move_next();
    }

    // Index any new particles
    auto new_particle_count = particles_.size();
    if(new_particle_count > original_particle_count) {
        auto added_count = new_particle_count - original_particle_count;
        for(uint32_t i = 0; i < added_count; ++i) {
            auto new_start = original_particle_count + (i * 4);
            index_data_->index(new_start + 0);
            index_data_->index(new_start + 1);
            index_data_->index(new_start + 2);

            index_data_->index(new_start + 0);
            index_data_->index(new_start + 2);
            index_data_->index(new_start + 3);
        }
    }

    // Truncate if necessary (in which case the above loop did nothing)
    index_data_->resize(new_particle_count * 6);

    vertex_buffer_dirty_ = true;
    vertex_data_->done();

    index_buffer_dirty_ = true;
    index_data_->done();
}

std::vector<Particle> ParticleEmitter::do_emit(float dt, uint32_t max) {
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
            dir = dir.random_deviant(ang);
        }

        p.velocity = dir.normalized() * random_gen::random_float(velocity_range().first, velocity_range().second);

        //We have to rotate the velocity by the system, because if the particle system is attached to something (e.g. the back of a spaceship)
        //when that entity rotates we want the velocity to stay pointing relative to the entity
        auto rot = system().absolute_rotation();

        p.velocity *= rot;


        p.ttl = random_gen::random_float(ttl_range().first, ttl_range().second);
        p.colour = colour();

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


void ParticleEmitter::set_ttl(float seconds) {
    ttl_range_ = std::make_pair(seconds, seconds);
}

void ParticleEmitter::set_ttl_range(float min_seconds, float max_seconds) {
    if(min_seconds > max_seconds) {
        throw std::logic_error("min_seconds can't be greater than max_seconds");
    }

    ttl_range_ = std::make_pair(min_seconds, max_seconds);
}

std::pair<float, float> ParticleEmitter::ttl_range() const {
    return ttl_range_;
}

void ParticleEmitter::set_repeat_delay(float seconds) {
    set_repeat_delay_range(seconds, seconds);
}

void ParticleEmitter::set_repeat_delay_range(float min_seconds, float max_seconds) {
    repeat_delay_range_ = std::make_pair(min_seconds, max_seconds);
}

std::pair<float, float> ParticleEmitter::repeat_delay_range() const {
    return repeat_delay_range_;
}

void ParticleEmitter::set_velocity(float vel) {
    set_velocity_range(vel, vel);
}

void ParticleEmitter::set_velocity_range(float min_vel, float max_vel) {
    velocity_range_ = std::make_pair(min_vel, max_vel);
}

std::pair<float, float> ParticleEmitter::velocity_range() const {
    return velocity_range_;
}

void ParticleEmitter::set_duration(float seconds) {
    set_duration_range(seconds, seconds);
}

void ParticleEmitter::set_duration_range(float min_seconds, float max_seconds) {
    duration_range_ = std::make_pair(min_seconds, max_seconds);
    current_duration_ = random_gen::random_float(duration_range_.first, duration_range_.second);
}

std::pair<float, float> ParticleEmitter::duration_range() const {
    return duration_range_;
}

void ParticleSystem::set_particle_width(float width) {
    particle_width_ = width;
}

void ParticleSystem::set_particle_height(float height) {
    particle_height_ = height;
}

}
