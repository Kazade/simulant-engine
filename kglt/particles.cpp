#include <kazbase/random.h>
#include "stage.h"
#include "particles.h"

#ifdef KGLT_GL_VERSION_2X
#include "buffer_object.h"
#endif

namespace kglt {

ParticleSystem::ParticleSystem(ParticleSystemID id, Stage* stage):
    generic::Identifiable<ParticleSystemID>(id),
    ParentSetterMixin<MoveableObject>(stage),
    Source(stage),
    vertex_data_(new VertexData(VertexSpecification::POSITION_AND_DIFFUSE)),
    index_data_(new IndexData()) {

#ifdef KGLT_GL_VERSION_2X
    vao_.reset(new VertexArrayObject(MODIFY_REPEATEDLY_USED_FOR_RENDERING, MODIFY_REPEATEDLY_USED_FOR_RENDERING));
#endif

    set_material_id(stage->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY));
}

ParticleSystem::~ParticleSystem() {
    delete vertex_data_;
    vertex_data_ = nullptr;

    delete index_data_;
    index_data_ = nullptr;
}

void ParticleSystem::set_material_id(MaterialID mat_id) {
    if(!mat_id) {
        throw ValueError("A particle system must always have a valid material");
    }

    material_id_ = mat_id;

    //Hold a reference to the material so that it's destroyed when we are
    material_ref_ = stage->material(material_id_);
}

EmitterPtr ParticleSystem::push_emitter() {
    auto new_emitter = std::make_shared<ParticleEmitter>(*this);
    emitters_.push_back(new_emitter);
    return new_emitter;
}

void ParticleSystem::pop_emitter() {
    emitters_.pop_back();
}

//Boundable entity things
const AABB ParticleSystem::aabb() const {
    if(emitters_.empty()) {
        return AABB();
    }

    AABB result;
    bool first = true;
    for(auto emitter: emitters_) {
        auto pos = emitter->relative_position();

        if(emitter->type() == PARTICLE_EMITTER_POINT) {            
            if(pos.x > result.max.x || first) result.max.x = pos.x;
            if(pos.y > result.max.y || first) result.max.y = pos.y;
            if(pos.z > result.max.z || first) result.max.z = pos.z;

            if(pos.x < result.min.x || first) result.min.x = pos.x;
            if(pos.y < result.min.x || first) result.min.y = pos.y;
            if(pos.z < result.min.x || first) result.min.z = pos.z;
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

            if(maxx > result.max.x || first) result.max.x = maxx;
            if(maxy > result.max.y || first) result.max.y = maxy;
            if(maxz > result.max.z || first) result.max.z = maxz;

            if(minx > result.min.x || first) result.min.x = minx;
            if(miny > result.min.y || first) result.min.y = miny;
            if(minz > result.min.z || first) result.min.z = minz;
        }

        first = false;
    }

    return result;
}

#ifdef KGLT_GL_VERSION_2X
void ParticleSystem::_update_vertex_array_object() {
    if(!index_data_->count()) {
        return;
    }

    vao_->vertex_buffer_update(vertex_data_->data_size(), vertex_data_->data());
    vao_->index_buffer_update(index_data_->count() * sizeof(uint16_t), index_data_->_raw_data());
}

void ParticleSystem::_bind_vertex_array_object() {
    vao_->bind();
}

#endif

const AABB ParticleSystem::transformed_aabb() const {
    AABB box = aabb(); //Get the untransformed one
    auto pos = absolute_position();
    kmVec3Add(&box.min, &box.min, &pos);
    kmVec3Add(&box.max, &box.max, &pos);
    return box;
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

void ParticleEmitter::update(double dt) {
    time_active_ += dt;

    if(current_duration_ && time_active_ >= current_duration_) {
        deactivate();

        float repeat_delay = random_gen::random_float(repeat_delay_range_.first, repeat_delay_range_.second);
        if(repeat_delay > 0) {
            system().stage->window->idle->add_timeout(repeat_delay, std::bind(&ParticleEmitter::activate, this));
        }
    }
}

void ParticleSystem::do_update(double dt) {
    update_source(dt); //Update any sounds attached to this particle system

    auto current_particle_count = particles_.size();
    auto original_particle_count = current_particle_count;

    for(auto emitter: emitters_) {
        emitter->update(dt);

        if(!emitter->is_active()) {
            continue;
        }

        auto max_can_emit = quota_ - current_particle_count;
        auto new_particles = emitter->do_emit(dt, max_can_emit);
        particles_.insert(particles_.end(), new_particles.begin(), new_particles.end());
        current_particle_count += new_particles.size();
    }

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

    if(particles_.empty() && !has_repeating_emitters() && !has_active_emitters()) {
        // If the particles are gone, and we don't have repeating emitters and all the emitters are inactive
        // Then destroy the particle system if that's what we've been told to do
        if(destroy_on_completion()) {
            ask_owner_for_destruction();
            // No point doing anything else!
            return;
        }
    }

    vertex_data_->move_to_start();
    vertex_data_->resize(particles_.size());
    for(auto particle: particles_) {
        vertex_data_->position(particle.position);
        vertex_data_->diffuse(particle.colour);
        vertex_data_->move_next();

    }

    // Index any new particles
    auto new_particle_count = particles_.size();
    for(uint32_t i = original_particle_count; i < new_particle_count; ++i) {
        index_data_->index(i);
    }

    // Truncate if necessary (in which case the above loop did nothing)
    index_data_->resize(new_particle_count);

    vertex_data_->done();
    index_data_->done();
}

std::vector<Particle> ParticleEmitter::do_emit(double dt, uint32_t max) {
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
        if(angle().value_ != 0) {
            Radians ang(angle()); //Convert from degress to radians
            ang.value_ *= random_gen::random_float(0, 1); //Multiply by a random unit float
            dir = dir.random_deviant(ang);
        }

        p.velocity = dir.normalized() * random_gen::random_float(velocity_range().first, velocity_range().second);

        //We have to rotate the velocity by the system, because if the particle system is attached to something (e.g. the back of a spaceship)
        //when that entity rotates we want the velocity to stay pointing relative to the entity
        auto rot = system().absolute_rotation();
        kmQuaternionMultiplyVec3(&p.velocity, &rot, &p.velocity);

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
        throw ValueError("min_seconds can't be greater than max_seconds");
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
    MoveableObject::stage->material(material_id())->pass(0)->set_point_size(width);
    particle_width_ = width;
}

}
