
#include "../asset_manager.h"
#include "particle_script.h"
#include "particles/curves.h"

namespace smlt {

const std::string ParticleScript::BuiltIns::FIRE = "particles/fire.kglp";

bool ParticleScript::has_repeating_emitters() const {
    for(auto i = 0u; i < emitter_count_; ++i) {
        auto& e = emitters_[i];
        auto range = e.repeat_delay_range;
        if(range.first > 0.0f || range.second > 0.0f) {
            return true;
        }
    }

    return false;
}

ParticleScript::ParticleScript(ParticleScriptID id, AssetManager* asset_manager):
    Asset(asset_manager),
    generic::Identifiable<ParticleScriptID>(id) {

    material_ = asset_manager->clone_default_material();
}

std::size_t ParticleScript::emitter_count() const {
    return emitter_count_;
}

const Emitter* ParticleScript::emitter(std::size_t i) const {
    if(i < emitters_.size()) {
        return &emitters_[i];
    } else {
        return nullptr;
    }
}

Emitter* ParticleScript::mutable_emitter(std::size_t i) {
    return (i < emitters_.size()) ? &emitters_[i] : nullptr;
}

std::size_t ParticleScript::manipulator_count() const {
    return manipulators_.size();
}

const Manipulator* ParticleScript::manipulator(std::size_t i) const {
    return manipulators_.at(i).get();
}

std::size_t ParticleScript::quota() const {
    return quota_;
}

float ParticleScript::particle_width() const {
    return particle_width_;
}

float ParticleScript::particle_height() const {
    return particle_height_;
}

bool ParticleScript::cull_each() const {
    return cull_each_;
}

MaterialPtr ParticleScript::material() const {
    return material_;
}

void Manipulator::set_linear_curve(float rate) {
    using namespace std::placeholders;
    curve_ = std::bind(&linear_curve, _1, _2, _3, rate);
}

void Manipulator::set_bell_curve(float peak, float deviation) {
    using namespace std::placeholders;
    curve_ = std::bind(&bell_curve, _1, _2, _3, peak, deviation);
}

void ParticleScript::add_manipulator(std::shared_ptr<Manipulator> manipulator) {
    manipulators_.push_back(manipulator);
}

void ParticleScript::push_emitter(const Emitter& emitter) {
    emitters_[emitter_count_++] = emitter;
}

void ParticleScript::clear_emitters() {
    emitter_count_ = 0;
}

void ParticleScript::set_quota(std::size_t quota) {
    quota_ = quota;
}

void ParticleScript::set_particle_width(float w) {
    particle_width_ = w;
}

void ParticleScript::set_particle_height(float h) {
    particle_height_ = h;
}

void ParticleScript::set_cull_each(bool v) {
    cull_each_ = v;
}

void ParticleScript::set_material(MaterialPtr material) {
    material_ = material;
}

}
