#pragma once

#include <memory>
#include <unordered_map>

#include "stage_node.h"

#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../generic/manual_object.h"
#include "../renderers/renderer.h"
#include "../sound.h"
#include "../interfaces.h"
#include "../types.h"
#include "../vertex_data.h"
#include "../utils/random.h"
#include "../assets/particle_script.h"

namespace smlt {

struct Particle {
    smlt::Vec3 position;
    smlt::Vec3 velocity;
    smlt::Vec2 dimensions;
    smlt::Vec2 initial_dimensions;
    float ttl;
    float lifetime;
    smlt::Colour colour;
};


class ParticleSystem;

typedef sig::signal<void (ParticleSystem*, MaterialID, MaterialID)> ParticleSystemMaterialChangedSignal;

class ParticleSystem :
    public TypedDestroyableObject<ParticleSystem, Stage>,
    public StageNode,
    public generic::Identifiable<ParticleSystemID>,
    public AudioSource,
    public Loadable,
    public HasMutableRenderPriority,
    public ChainNameable<ParticleSystem>  {

    DEFINE_SIGNAL(ParticleSystemMaterialChangedSignal, signal_material_changed);

public:
    ParticleSystem(Stage* stage, SoundDriver *sound_driver, ParticleScriptPtr script);
    virtual ~ParticleSystem();

    const AABB& aabb() const override;
    const AABB transformed_aabb() const override {
        return StageNode::transformed_aabb();
    }

    bool emitters_active() const;
    void set_emitters_active(bool value=true);

    void set_destroy_on_completion(bool value=true) { destroy_on_completion_ = value; }
    bool destroy_on_completion() const { return destroy_on_completion_; }
    bool has_active_emitters() const;

    /**
     * @brief If update_when_hidden is true, then particles will continue to be simulated
     * even if the particle system is hidden via set_visible. The default is false.
     * @return True if update_when_hidden is set.
     */
    bool update_when_hidden() const;
    void set_update_when_hidden(bool value=true);

    VertexData* vertex_data() const {
        return vertex_data_;
    }

    IndexData* index_data() const {
        return index_data_;
    }

    void clean_up() override {
        StageNode::clean_up();
    }

    void _get_renderables(batcher::RenderQueue* render_queue, const CameraPtr camera, const DetailLevel detail_level) override;

    ParticleScript* script() const {
        return script_.get();
    }

    void update(float dt) override;

    std::size_t particle_count() const {
        return particle_count_;
    }

    const Particle& particle(const std::size_t i) const {
        return particles_[i];
    }

private:
    struct EmitterState {
        bool is_active = true;
        float time_active = 0.0f;
        float time_inactive = 0.0f;
        float current_duration = 0.0f;
        float repeat_delay = 0.0f;
        float emission_accumulator = 0.0f;
    };

    std::array<EmitterState, ParticleScript::MAX_EMITTER_COUNT> emitter_states_;

    void update_emitter(uint16_t emitter, float dt);
    void update_active_state(uint16_t emitter, float dt);
    void emit_particles(uint16_t emitter, float dt, uint32_t max);


    AABB aabb_;
    void calc_aabb();

    ParticleScriptPtr script_;

    std::vector<Particle> particles_;
    std::size_t particle_count_ = 0;

    VertexData* vertex_data_ = nullptr;
    IndexData* index_data_ = nullptr;

    bool destroy_on_completion_ = false;
    bool update_when_hidden_ = false;

    void rebuild_vertex_data(const smlt::Vec3& up, const smlt::Vec3& right);

    bool emitters_active_ = true;

    RandomGenerator random_;
};

}
