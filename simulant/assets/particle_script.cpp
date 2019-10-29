
#include "../asset_manager.h"
#include "particle_script.h"
#include "particles/curves.h"

namespace smlt {

class ParticleScriptImpl {
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

    std::size_t quota_ = 0;
    float particle_width_ = 100.0f;
    float particle_height_ = 100.0f;
    bool cull_each_ = false;

    std::array<Emitter, ParticleScript::MAX_EMITTER_COUNT> emitters_;
    uint16_t emitter_count_ = 0;

    std::vector<ManipulatorPtr> manipulators_;
    MaterialPtr material_;
};

ParticleScript::ParticleScript(ParticleScriptID id, AssetManager* asset_manager):
    Asset(asset_manager),
    AtomicAsset<ParticleScript, ParticleScriptImpl, ParticleScriptTransaction>(
        std::make_shared<ParticleScriptImpl>(this)
    ),
    generic::Identifiable<ParticleScriptID>(id) {

    pimpl_->material_ = asset_manager->clone_default_material();
}

std::size_t ParticleScript::emitter_count() const {
    return pimpl_->emitters_.size();
}

const Emitter* ParticleScript::emitter(std::size_t i) const {
    if(i < pimpl_->emitters_.size()) {
        return &pimpl_->emitters_[i];
    } else {
        return nullptr;
    }
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

}
