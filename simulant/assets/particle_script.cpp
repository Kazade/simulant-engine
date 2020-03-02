
#include "../asset_manager.h"
#include "particle_script.h"
#include "particles/curves.h"

namespace smlt {

const std::string ParticleScript::BuiltIns::FIRE = "simulant/particles/fire.kglp";

class ParticleScriptImpl {
    friend class ParticleScriptTransaction;
    friend class ParticleScript;

public:
    ParticleScriptImpl(ParticleScript* owner):
        owner_(owner) {}

    bool has_repeating_emitters() const {
        for(auto& e: emitters_) {
            auto range = e.repeat_delay_range;
            if(range.first || range.second) {
                return true;
            }
        }

        return false;
    }

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

std::shared_ptr<ParticleScriptImpl> ParticleScript::clone_impl() {
    return std::make_shared<ParticleScriptImpl>(*pimpl_);
}

ParticleScript::ParticleScript(ParticleScriptID id, AssetManager* asset_manager):
    Asset(asset_manager),
    AtomicAsset<ParticleScript, ParticleScriptImpl, ParticleScriptTransaction>(
        std::make_shared<ParticleScriptImpl>(this)
    ),
    generic::Identifiable<ParticleScriptID>(id) {

    pimpl_->material_ = asset_manager->clone_default_material();
}

std::size_t ParticleScript::emitter_count() const {
    return pimpl_->emitter_count_;
}

const Emitter* ParticleScript::emitter(std::size_t i) const {
    if(i < pimpl_->emitters_.size()) {
        return &pimpl_->emitters_[i];
    } else {
        return nullptr;
    }
}

std::size_t ParticleScript::manipulator_count() const {
    return pimpl_->manipulators_.size();
}

const Manipulator* ParticleScript::manipulator(std::size_t i) const {
    return pimpl_->manipulators_.at(i).get();
}

std::size_t ParticleScript::quota() const {
    return pimpl_->quota_;
}

float ParticleScript::particle_width() const {
    return pimpl_->particle_width_;
}

float ParticleScript::particle_height() const {
    return pimpl_->particle_height_;
}

bool ParticleScript::cull_each() const {
    return pimpl_->cull_each_;
}

MaterialPtr ParticleScript::material() const {
    return pimpl_->material_;
}

bool ParticleScript::has_repeating_emitters() const {
    return pimpl_->has_repeating_emitters();
}

void Manipulator::set_linear_curve(float rate) {
    using namespace std::placeholders;
    curve_ = std::bind(&linear_curve, _1, _2, _3, rate);
}

void Manipulator::set_bell_curve(float peak, float deviation) {
    using namespace std::placeholders;
    curve_ = std::bind(&bell_curve, _1, _2, _3, peak, deviation);
}

void ParticleScriptTransaction::add_manipulator(std::shared_ptr<Manipulator> manipulator) {
    target_->manipulators_.push_back(manipulator);
}

void ParticleScriptTransaction::push_emitter(const Emitter& emitter) {
    target_->emitters_[target_->emitter_count_++] = emitter;
}

void ParticleScriptTransaction::clear_emitters() {
    target_->emitter_count_ = 0;
}

void ParticleScriptTransaction::set_name(const std::string& name) {
    target_->name_ = name;
}

void ParticleScriptTransaction::set_quota(std::size_t quota) {
    target_->quota_ = quota;
}

void ParticleScriptTransaction::set_particle_width(float w) {
    target_->particle_width_ = w;
}

void ParticleScriptTransaction::set_particle_height(float h) {
    target_->particle_height_ = h;
}

void ParticleScriptTransaction::set_cull_each(bool v) {
    target_->cull_each_ = v;
}

void ParticleScriptTransaction::set_material(MaterialPtr material) {
    target_->material_ = material;
}

}
