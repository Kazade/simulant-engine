#include "particle_system.h"

#include "particles/emitter.h"

#include "../renderers/batching/renderable_store.h"
#include "../frustum.h"
#include "../stage.h"
#include "../types.h"
#include "camera.h"

namespace smlt {

using smlt::particles::Emitter;
using smlt::particles::EmitterPtr;
using smlt::particles::Particle;
using smlt::particles::PARTICLE_EMITTER_BOX;
using smlt::particles::PARTICLE_EMITTER_POINT;


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

    set_quota(INITIAL_QUOTA); // Force hardware buffer initialization
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
    auto new_emitter = std::make_shared<Emitter>(*this);
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

void ParticleSystem::_get_renderables(RenderableFactory* factory, CameraPtr camera, DetailLevel detail_level) {
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
    new_renderable.material_id = material_id_;
    new_renderable.centre = transformed_aabb().centre();

    factory->push_renderable(new_renderable);
}

void ParticleSystem::destroy() {
    stage->destroy_particle_system(id());
}

void ParticleSystem::set_quota(std::size_t quota) {
    if(quota == quota_) {
        return;
    }

    quota_ = quota;

    // Shrink if necessary
    if(particles_.size() > quota) {
        particles_.resize(quota);
        particles_.shrink_to_fit();
    } else {
        // Reserve space for all the particles
        particles_.reserve(quota);
    }
}

void ParticleSystem::rebuild_vertex_data(const smlt::Vec3& up, const smlt::Vec3& right) {
    vertex_data_->resize(quota_ * 4);
    vertex_data_->move_to_start();

    /* FIXME: Remove this when #193 is complete */
    index_data_->resize(quota_ * 4);
    index_data_->clear();

    auto i = 0;
    for(auto& p: particles_) {
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

    // Update existing particles, erase any that are dead
    for(auto& particle: particles_) {
        particle.position += particle.velocity * dt;
        particle.ttl -= dt;
    }

    // Erase dead particles
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p) -> bool { return p.ttl <= 0.0f; }),
        particles_.end()
    );

    // Run any manipulations on the particles, we do this before
    // we add new particles - otherwise they get manipulated before they're
    // even displayed!
    for(auto& manipulator: manipulators_) {
        manipulator->manipulate(particles_, dt);
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
        emitter->do_emit(dt, max_can_emit, particles_);
    }

    if(particles_.empty() && !has_repeating_emitters() && !has_active_emitters()) {
        // If the particles are gone, and we don't have repeating emitters and all the emitters are inactive
        // Then destroy the particle system if that's what we've been told to do
        if(destroy_on_completion()) {
            destroy();
            // No point doing anything else!
            return;
        }
    }
}

void ParticleSystem::set_particle_width(float width) {
    particle_width_ = width;
}

void ParticleSystem::set_particle_height(float height) {
    particle_height_ = height;
}

}
