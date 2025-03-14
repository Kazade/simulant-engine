#include "particle_system.h"

#include "../frustum.h"
#include "../meshes/submesh.h"
#include "../stage.h"
#include "../types.h"
#include "camera.h"

namespace smlt {

const static VertexSpecification
    PS_VERTEX_SPEC(smlt::VERTEX_ATTRIBUTE_3F, // Position
                   smlt::VERTEX_ATTRIBUTE_NONE,
                   smlt::VERTEX_ATTRIBUTE_2F, // Texcoord 0
                   smlt::VERTEX_ATTRIBUTE_NONE, smlt::VERTEX_ATTRIBUTE_NONE,
                   smlt::VERTEX_ATTRIBUTE_NONE, smlt::VERTEX_ATTRIBUTE_NONE,
                   smlt::VERTEX_ATTRIBUTE_NONE, smlt::VERTEX_ATTRIBUTE_NONE,
                   smlt::VERTEX_ATTRIBUTE_NONE,
#ifdef __DREAMCAST__
                   smlt::VERTEX_ATTRIBUTE_4UB_BGRA // Diffuse
#else
                   smlt::VERTEX_ATTRIBUTE_4F // Diffuse
#endif
    );

ParticleSystem::ParticleSystem(Scene* owner) :
    StageNode(owner, STAGE_NODE_TYPE_PARTICLE_SYSTEM),
    vertex_data_(new VertexData(PS_VERTEX_SPEC)) {}

ParticleSystem::~ParticleSystem() {
    delete vertex_data_;
    vertex_data_ = nullptr;
}

void ParticleSystem::calc_aabb() {
    if(!script_->emitter_count()) {
        aabb_ = AABB();
        return;
    }

    AABB& result = aabb_;
    result = AABB();

    for(auto e = 0u; e < script_->emitter_count(); ++e) {
        auto emitter = script_->emitter(e);
        auto pos = emitter->relative_position;

        if(emitter->type == PARTICLE_EMITTER_POINT) {
            if(e == 0) {
                result.set_min_max(pos, pos);
            } else {
                result.encapsulate(pos);
            }
        } else {
            // If this is not a point emitter, then calculate the max/min
            // possible for each emitter using their dimensions.

            float hw = smlt::fast_divide(emitter->dimensions.x, 2.0f);
            float hh = smlt::fast_divide(emitter->dimensions.y, 2.0f);
            float hd = smlt::fast_divide(emitter->dimensions.z, 2.0f);

            AABB emitter_bounds(pos, Vec3(hw, hh, hd));
            if(e == 0) {
                result = emitter_bounds;
            } else {
                result.encapsulate(emitter_bounds);
            }
        }
    }
}

// Boundable entity things
const AABB& ParticleSystem::aabb() const {
    return aabb_;
}

bool ParticleSystem::emitters_active() const {
    return emitters_active_;
}

void ParticleSystem::set_emitters_active(bool value) {
    emitters_active_ = value;
}

bool ParticleSystem::has_active_emitters() const {
    if(!emitters_active_) {
        return false;
    }

    for(auto i = 0u; i < script_->emitter_count(); ++i) {
        auto& e = emitter_states_[i];
        if(e.is_active) {
            return true;
        }
    }

    return false;
}

bool ParticleSystem::update_when_hidden() const {
    return update_when_hidden_;
}

void ParticleSystem::set_update_when_hidden(bool value) {
    update_when_hidden_ = value;
}

void ParticleSystem::do_generate_renderables(batcher::RenderQueue* render_queue,
                                             const Camera* camera,
                                             const Viewport*,
                                             const DetailLevel detail_level,
                                             Light** lights,
                                             const std::size_t light_count) {
    _S_UNUSED(detail_level);

    if(!is_visible()) {
        return;
    }

    if(!particle_count()) {
        // No particles, no renderables!
        return;
    }

    /* Rebuild the vertex data with the current camera direction */
    rebuild_vertex_data(camera->transform->up(), camera->transform->right());

    Renderable new_renderable;
    new_renderable.arrangement = MESH_ARRANGEMENT_TRIANGLE_STRIP;
    new_renderable.render_priority = render_priority();
    new_renderable.final_transformation = Mat4();
    new_renderable.index_data = nullptr;
    new_renderable.index_element_count = 0;
    new_renderable.vertex_range_count = vertex_ranges_.size();
    new_renderable.vertex_ranges = vertex_ranges_.data();
    new_renderable.vertex_data = vertex_data_;
    new_renderable.is_visible = true;
    new_renderable.material = script_->material().get();
    new_renderable.center = transformed_aabb().center();

    new_renderable.light_count = light_count;
    for(std::size_t i = 0; i < light_count; ++i) {
        new_renderable.lights_affecting_this_frame[i] = lights[i];
    }

    render_queue->insert_renderable(std::move(new_renderable));
}

void ParticleSystem::rebuild_vertex_data(const smlt::Vec3& up,
                                         const smlt::Vec3& right) {
    vertex_data_->resize(particle_count_ * 4);
    vertex_data_->move_to_start();

    vertex_ranges_.resize(0);

    if(!particle_count_) {
        return;
    }

    uint8_t* pos_ptr = vertex_data_->data();
    uint8_t* dif_ptr =
        pos_ptr + vertex_data_->vertex_specification().diffuse_offset(false);
    uint8_t* uv_ptr =
        pos_ptr + vertex_data_->vertex_specification().texcoord0_offset(false);

    auto stride = vertex_data_->vertex_specification().stride();

    for(auto j = 0u; j < particle_count_; ++j) {
        auto& p = particles_[j];

        Vec3* pos = (Vec3*)pos_ptr;
        float* uv = (float*)uv_ptr;

#ifdef __DREAMCAST__
        uint8_t* dif = dif_ptr;
        uint8_t a = smlt::clamp(p.color.a * 255.0f, 0, 255);
        uint8_t r = smlt::clamp(p.color.r * 255.0f, 0, 255);
        uint8_t g = smlt::clamp(p.color.g * 255.0f, 0, 255);
        uint8_t b = smlt::clamp(p.color.b * 255.0f, 0, 255);

#define RIDX 2
#define GIDX 1
#define BIDX 0
#define AIDX 3
#else
        float* dif = (float*)dif_ptr;
        float a = p.color.a;
        float r = p.color.r;
        float g = p.color.g;
        float b = p.color.b;

#define RIDX 0
#define GIDX 1
#define BIDX 2
#define AIDX 3
#endif

        *(pos) = p.position + right * p.dimensions.x * -0.5f +
                 up * p.dimensions.y * -0.5f;

        dif[BIDX] = b;
        dif[GIDX] = g;
        dif[RIDX] = r;
        dif[AIDX] = a;

        uv[0] = 0.0f;
        uv[1] = 0.0f;

        pos_ptr += stride;
        dif_ptr += stride;
        uv_ptr += stride;

        pos = (Vec3*)pos_ptr;
        dif = (decltype(dif))dif_ptr;
        uv = (float*)uv_ptr;

        *(pos) = p.position + right * p.dimensions.x * +0.5f +
                 up * p.dimensions.y * -0.5f;

        dif[BIDX] = b;
        dif[GIDX] = g;
        dif[RIDX] = r;
        dif[AIDX] = a;

        uv[0] = 1.0f;
        uv[1] = 0.0f;

        pos_ptr += stride;
        dif_ptr += stride;
        uv_ptr += stride;

        pos = (Vec3*)pos_ptr;
        dif = (decltype(dif))dif_ptr;
        uv = (float*)uv_ptr;

        *(pos) = p.position + right * p.dimensions.x * -0.5f +
                 up * p.dimensions.y * +0.5f;

        dif[BIDX] = b;
        dif[GIDX] = g;
        dif[RIDX] = r;
        dif[AIDX] = a;

        uv[0] = 0.0f;
        uv[1] = 1.0f;

        pos_ptr += stride;
        dif_ptr += stride;
        uv_ptr += stride;

        pos = (Vec3*)pos_ptr;
        dif = (decltype(dif))dif_ptr;
        uv = (float*)uv_ptr;

        *(pos) = p.position + right * p.dimensions.x * +0.5f +
                 up * p.dimensions.y * +0.5f;

        dif[BIDX] = b;
        dif[GIDX] = g;
        dif[RIDX] = r;
        dif[AIDX] = a;

        uv[0] = 1.0f;
        uv[1] = 1.0f;

        pos_ptr += stride;
        dif_ptr += stride;
        uv_ptr += stride;

        /* Add a vertex range for this tri-strip */
        VertexRange new_range;
        new_range.start = j * 4;
        new_range.count = 4;
        vertex_ranges_.push_back(new_range);
    }

    vertex_data_->done();
}

void ParticleSystem::on_update(float dt) {
    /* Don't update anything at all if we're hidden */
    if(!is_visible() && !update_when_hidden()) {
        return;
    }

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
        if(!emitter_states_[i].is_active) {
            continue;
        }

        update_emitter(i, dt);

        /* FIXME: This always means the first emitter gets all the particles !
         */
        auto max_can_emit = particles_.size() - particle_count_;
        emit_particles(i, dt, max_can_emit);

        // We do this after emission so that we always emit particles
        // on the first update
        update_active_state(i, dt);
    }

    /* If we are set to destroy on completion, then we do so even if we're
     * invisible */
    if(!particle_count_ && !script_->has_repeating_emitters() &&
       !has_active_emitters()) {
        // If the particles are gone, and we don't have repeating emitters and
        // all the emitters are inactive Then destroy the particle system if
        // that's what we've been told to do
        if(destroy_on_completion()) {
            destroy();
        }
    }
}

bool ParticleSystem::on_create(Params params) {
    if(!clean_params<ParticleSystem>(params)) {
        return false;
    }

    auto maybe_script = params.get<ParticleScriptPtr>("script");
    if(!maybe_script) {
        return false;
    }

    script_ = maybe_script.value();

    // Initialize the emitter states
    for(auto i = 0u; i < script_->emitter_count(); ++i) {
        auto emitter = script_->emitter(i);
        emitter_states_[i].current_duration = random_.float_in_range(
            emitter->duration_range.first, emitter->duration_range.second);

        emitter_states_[i].emission_accumulator = 0.0f;
    }

    return true;
}

void ParticleSystem::update_active_state(uint16_t e, float dt) {
    auto& state = emitter_states_[e];
    auto emitter = script_->emitter(e);

    if(state.is_active) {
        state.time_active += dt;

        if(state.current_duration > smlt::EPSILON &&
           state.time_active >= state.current_duration) {
            state.is_active = false;
            state.repeat_delay =
                random_.float_in_range(emitter->repeat_delay_range.first,
                                       emitter->repeat_delay_range.second);
            state.time_inactive = 0;
        }
    } else {
        state.time_inactive += dt;

        if(state.repeat_delay > smlt::EPSILON &&
           state.time_inactive >= state.repeat_delay) {
            state.is_active = true;
            state.repeat_delay = 0;
            state.time_active = 0;

            // Reset the length of this round of emission
            state.current_duration = random_.float_in_range(
                emitter->duration_range.first, emitter->duration_range.second);
            state.emission_accumulator = 0;
        }
    }
}

void ParticleSystem::update_emitter(uint16_t e, float dt) {
    auto& state = emitter_states_[e];

    if(state.is_active) {
        state.emission_accumulator += dt; // Buffer time
    }
}

void ParticleSystem::emit_particles(uint16_t e, float dt, uint32_t max) {
    _S_UNUSED(dt);

    if(!emitters_active_) {
        // If emitters were disabled, don't emit!
        return;
    }

    if(!max) {
        return; // Do nothing
    }

    auto& state = emitter_states_[e];
    auto emitter = script_->emitter(e);

    /* FIXME: Add smlt::fast_inverse() and use that */
    float decrement = smlt::fast_divide(
        1.0f,
        float(emitter->emission_rate)); // Work out how often to emit per second

    auto scale = transform->scale_factor();

    uint32_t to_emit = max;
    while(state.emission_accumulator >= decrement) {
        // EMIT THE PARTICLE!
        Particle p;
        if(emitter->type == PARTICLE_EMITTER_POINT) {
            p.position = transform->position() + emitter->relative_position;
        } else {
            p.position = transform->position() + emitter->relative_position;

            float hw = emitter->dimensions.x * 0.5f * scale.x;
            float hh = emitter->dimensions.y * 0.5f * scale.y;
            float hd = emitter->dimensions.z * 0.5f * scale.z;

            p.position.x += random_.float_in_range(-hw, hw);
            p.position.y += random_.float_in_range(-hh, hh);
            p.position.z += random_.float_in_range(-hd, hd);
        }

        // We have to rotate the velocity by the system, because if the particle
        // system is attached to something (e.g. the back of a spaceship) when
        // that entity rotates we want the velocity to stay pointing relative to
        // the entity
        auto rot = transform->orientation();

        Vec3 dir = emitter->direction;
        if(smlt::almost_equal(emitter->angle.to_float(), 360.0f)) {
            dir = smlt::Vec3(
                      RandomGenerator::instance().float_in_range(-1.0f, 1.0f),
                      RandomGenerator::instance().float_in_range(-1.0f, 1.0f),
                      RandomGenerator::instance().float_in_range(-1.0f, 1.0f))
                      .normalized();
        } else if(emitter->angle.to_float() != 0) {
            Degrees ang(emitter->angle);
            dir = dir.random_deviant(ang);
            dir *= rot;
        }

        p.velocity = dir *
                     random_.float_in_range(emitter->velocity_range.first,
                                            emitter->velocity_range.second) *
                     scale;

        p.lifetime = p.ttl = random_.float_in_range(emitter->ttl_range.first,
                                                    emitter->ttl_range.second);
        p.color = random_.choice(emitter->colors);
        p.initial_dimensions = p.dimensions =
            smlt::Vec2(script_->particle_width() * scale.x,
                       script_->particle_height() * scale.y);

        // FIXME: Initialize other properties
        particles_[particle_count_++] = p;

        assert(particle_count_ <= particles_.size());

        state.emission_accumulator -=
            decrement; // Decrement the accumulator while we can
        to_emit--;
        if(!to_emit) {
            break;
        }
    }
}

} // namespace smlt
