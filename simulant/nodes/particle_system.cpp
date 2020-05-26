#include "particle_system.h"

#include "../frustum.h"
#include "../stage.h"
#include "../types.h"
#include "camera.h"

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

ParticleSystem::ParticleSystem(Stage* stage, SoundDriver* sound_driver, ParticleScriptPtr script):
    TypedDestroyableObject<ParticleSystem, Stage>(stage),
    StageNode(stage, STAGE_NODE_TYPE_PARTICLE_SYSTEM),
    Source(stage, sound_driver),
    script_(script),
    vertex_data_(new VertexData(PS_VERTEX_SPEC)),
    index_data_(new IndexData(INDEX_TYPE_16_BIT)) {

    // Initialize the emitter states
    for(auto i = 0u; i < script_->emitter_count(); ++i) {
        auto emitter = script_->emitter(i);
        emitter_states_[i].current_duration = random_.float_in_range(
            emitter->duration_range.first, emitter->duration_range.second
        );

        emitter_states_[i].emission_accumulator = 0.0f;
    }
}

ParticleSystem::~ParticleSystem() {
    delete vertex_data_;
    vertex_data_ = nullptr;

    delete index_data_;
    index_data_ = nullptr;
}

void ParticleSystem::calc_aabb() {
    if(!script_->emitter_count()) {
        aabb_ = AABB();
        return;
    }

    AABB& result = aabb_;
    bool first = true;
    for(auto e = 0u; e < script_->emitter_count(); ++e) {
        auto emitter = script_->emitter(e);
        auto pos = emitter->relative_position;

        if(emitter->type == PARTICLE_EMITTER_POINT) {
            if(pos.x > result.max().x || first) result.set_max_x(pos.x);
            if(pos.y > result.max().y || first) result.set_max_y(pos.y);
            if(pos.z > result.max().z || first) result.set_max_z(pos.z);

            if(pos.x < result.min().x || first) result.set_min_x(pos.x);
            if(pos.y < result.min().x || first) result.set_min_y(pos.y);
            if(pos.z < result.min().x || first) result.set_min_z(pos.z);
        } else {
            // If this is not a point emitter, then calculate the max/min possible for
            // each emitter using their dimensions.

            float hw = (emitter->dimensions.x / 2.0f);
            float hh = (emitter->dimensions.y / 2.0f);
            float hd = (emitter->dimensions.z / 2.0f);

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


bool ParticleSystem::has_active_emitters() const {
    for(auto e: emitter_states_) {
        if(e.is_active) {
            return true;
        }
    }

    return false;
}

void ParticleSystem::_get_renderables(batcher::RenderQueue* render_queue, const CameraPtr camera, const DetailLevel detail_level) {
    _S_UNUSED(detail_level);

    /* Rebuild the vertex data with the current camera direction */
    rebuild_vertex_data(camera->up(), camera->right());

    Renderable new_renderable;
    new_renderable.arrangement = MESH_ARRANGEMENT_QUADS;
    new_renderable.render_priority = render_priority();
    new_renderable.final_transformation = Mat4();
    new_renderable.index_data = index_data_;
    new_renderable.vertex_data = vertex_data_;
    new_renderable.index_element_count = index_data_->count();
    new_renderable.is_visible = is_visible();
    new_renderable.material = script_->material().get();
    new_renderable.centre = transformed_aabb().centre();

    render_queue->insert_renderable(std::move(new_renderable));
}

void ParticleSystem::rebuild_vertex_data(const smlt::Vec3& up, const smlt::Vec3& right) {
    vertex_data_->resize(particle_count_ * 4);
    vertex_data_->move_to_start();

    /* FIXME: Remove this when #193 is complete */
    index_data_->resize(particle_count_ * 4);
    index_data_->clear();

    auto i = 0;
    for(auto j = 0u; j < particle_count_; ++j) {
        auto& p = particles_[j];

        auto scaled_up = up * p.dimensions.y;
        auto scaled_right = right * p.dimensions.x;
        auto half_up = scaled_up * 0.5f;
        auto half_right = scaled_right * 0.5f;

        auto pos = p.position;
        pos += -half_up;
        pos += -half_right;

        vertex_data_->position(pos);
        vertex_data_->diffuse(p.colour);
        vertex_data_->tex_coord0(0, 0);
        vertex_data_->move_next();

        pos += scaled_right;
        vertex_data_->position(pos);
        vertex_data_->diffuse(p.colour);
        vertex_data_->tex_coord0(1, 0);
        vertex_data_->move_next();

        pos += scaled_up;
        vertex_data_->position(pos);
        vertex_data_->diffuse(p.colour);
        vertex_data_->tex_coord0(1, 1);
        vertex_data_->move_next();

        pos += -scaled_right;
        vertex_data_->position(pos);
        vertex_data_->diffuse(p.colour);
        vertex_data_->tex_coord0(0, 1);
        vertex_data_->move_next();

        index_data_->index(i++);
        index_data_->index(i++);
        index_data_->index(i++);
        index_data_->index(i++);
    }

    vertex_data_->done();
    index_data_->done();
}

void ParticleSystem::update(float dt) {
    update_source(dt); //Update any sounds attached to this particle system

    if(particles_.size() != script_->quota()) {
        particles_.resize(script_->quota());
        particles_.shrink_to_fit();
        particle_count_ = std::min(particle_count_, particles_.size());
    }


    // Update existing particles, erase any that are dead
    for(auto i = 0u; i < particle_count_; ++i) {
        Particle& particle = particles_[i];

        particle.position += particle.velocity * dt;
        particle.ttl -= dt;

        if(particle.ttl <= 0) {
            // Swap dead particles to the end of the
            // vector and decrease the particle count
            // this means active particles are at the
            // start of the vector
            std::swap(particles_[i], particles_[particle_count_ - 1]);

            --particle_count_;
            assert(particle_count_ < particles_.size());
        }
    }

    // Run any manipulations on the particles, we do this before
    // we add new particles - otherwise they get manipulated before they're
    // even displayed!
    for(auto i = 0u; i < script_->manipulator_count(); ++i) {
        auto manipulator = script_->manipulator(i);
        manipulator->manipulate(this, &particles_[0], particle_count_, dt);
    }

    for(auto i = 0u; i < script_->emitter_count(); ++i) {
        update_emitter(i, dt);

        if(!emitter_states_[i].is_active) {
            continue;
        }

        if(particle_count_ >= particles_.size()) {
            continue;
        }

        auto max_can_emit = particles_.size() - particle_count_;
        emit_particles(i, dt, max_can_emit);

        // We do this after emission so that we always emit particles
        // on the first update
        update_active_state(i, dt);
    }

    if(!particle_count_ && !script_->has_repeating_emitters() && !has_active_emitters()) {
        // If the particles are gone, and we don't have repeating emitters and all the emitters are inactive
        // Then destroy the particle system if that's what we've been told to do
        if(destroy_on_completion()) {
            destroy();
            // No point doing anything else!
            return;
        }
    }
}

void ParticleSystem::update_active_state(uint16_t e, float dt) {
    auto& state = emitter_states_[e];
    auto emitter = script_->emitter(e);

    if(state.is_active) {
        state.time_active += dt;

        if(state.current_duration && state.time_active >= state.current_duration) {
            state.is_active = false;
            state.repeat_delay = random_.float_in_range(emitter->repeat_delay_range.first, emitter->repeat_delay_range.second);
            state.time_inactive = 0;
        }
    } else {
        state.time_inactive += dt;

        if(state.repeat_delay > 0 && state.time_inactive >= state.repeat_delay) {
            state.is_active = true;
            state.repeat_delay = 0;
            state.time_active = 0;

            // Reset the length of this round of emission
            state.current_duration = random_.float_in_range(emitter->duration_range.first, emitter->duration_range.second);
            state.emission_accumulator = 0;
        }
    }
}

void ParticleSystem::update_emitter(uint16_t e, float dt) {
    auto& state = emitter_states_[e];

    if(state.is_active) {
        state.emission_accumulator += dt; //Buffer time
    }
}

void ParticleSystem::emit_particles(uint16_t e, float dt, uint32_t max) {
    _S_UNUSED(dt);

    if(!max) {
        return; //Do nothing
    }

    auto& state = emitter_states_[e];
    auto emitter = script_->emitter(e);

    float decrement = 1.0f / float(emitter->emission_rate); //Work out how often to emit per second

    auto scale = absolute_scaling();

    uint32_t to_emit = max;
    while(state.emission_accumulator >= decrement) {
        //EMIT THE PARTICLE!
        Particle p;
        if(emitter->type == PARTICLE_EMITTER_POINT) {
            p.position = absolute_position() + emitter->relative_position;
        } else {
            p.position = absolute_position() + emitter->relative_position;

            float hw = emitter->dimensions.x * 0.5f * scale.x;
            float hh = emitter->dimensions.y * 0.5f * scale.y;
            float hd = emitter->dimensions.z * 0.5f * scale.z;

            p.position.x += random_.float_in_range(-hw, hw);
            p.position.y += random_.float_in_range(-hh, hh);
            p.position.z += random_.float_in_range(-hd, hd);
        }

        Vec3 dir = emitter->direction;
        if(emitter->angle.value != 0) {
            Radians ang(emitter->angle); //Convert from degress to radians
            ang.value *= random_.float_in_range(0, 1); //Multiply by a random unit float
            dir = dir.random_deviant(ang).normalized();
        }

        p.velocity = dir * random_.float_in_range(emitter->velocity_range.first, emitter->velocity_range.second) * scale;

        //We have to rotate the velocity by the system, because if the particle system is attached to something (e.g. the back of a spaceship)
        //when that entity rotates we want the velocity to stay pointing relative to the entity
        auto rot = absolute_rotation();

        p.velocity *= rot;

        p.lifetime = p.ttl = random_.float_in_range(emitter->ttl_range.first, emitter->ttl_range.second);
        p.colour = emitter->colour;
        p.initial_dimensions = p.dimensions = smlt::Vec2(script_->particle_width() * scale.x, script_->particle_height() * scale.y);

        //FIXME: Initialize other properties
        particles_[particle_count_++] = p;

        assert(particle_count_ <= particles_.size());

        state.emission_accumulator -= decrement; //Decrement the accumulator while we can
        to_emit--;
        if(!to_emit) {
            break;
        }
    }
}

}
