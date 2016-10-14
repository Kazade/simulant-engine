#ifndef PARTICLES_H
#define PARTICLES_H

#include <memory>
#include <unordered_map>

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "renderers/renderer.h"
#include "utils/parent_setter_mixin.h"

#include "sound.h"
#include "object.h"
#include "interfaces.h"
#include "types.h"
#include "vertex_data.h"


namespace kglt {

enum ParticleEmitterType {
    PARTICLE_EMITTER_POINT,
    PARTICLE_EMITTER_BOX
};

const std::unordered_map<unicode, ParticleEmitterType> EMITTER_LOOKUP = {
    { "point", PARTICLE_EMITTER_POINT },
    { "box", PARTICLE_EMITTER_BOX }
};

struct Particle {
    kglt::Vec3 position;
    kglt::Vec3 velocity;
    float ttl;
    kglt::Colour colour;
};

class ParticleSystem;

class ParticleEmitter {
public:
    ParticleEmitter(ParticleSystem& system):
        system_(system) {}

    void set_type(ParticleEmitterType type) { type_ = type; }
    ParticleEmitterType type() const { return type_; }

    //Relative to the particle system
    kglt::Vec3 relative_position() const {
        return relative_position_;
    }

    void set_direction(const kglt::Vec3& dir) { direction_ = dir; }
    kglt::Vec3 direction() const {
        return direction_;
    }

    void set_angle(const Degrees& degs) { angle_ = degs; }
    Degrees angle() const {
        return angle_;
    }

    void set_colour(const kglt::Colour& col) { colour_ = col; }
    kglt::Colour colour() const {
        return colour_;
    }

    void set_emission_rate(const int rate) { emission_rate_ = rate; }
    int emission_rate() const {
        return emission_rate_;
    }

    void set_width(float w) { dimensions_.x = w; }
    void set_height(float h) { dimensions_.y = h; }
    void set_depth(float d) { dimensions_.z = d; }

    float width() const { return dimensions_.x; }
    float height() const { return dimensions_.y; }
    float depth() const { return dimensions_.z; }

    void set_ttl(float seconds);
    void set_ttl_range(float min_seconds, float max_seconds);
    std::pair<float, float> ttl_range() const;

    void set_repeat_delay(float seconds);
    void set_repeat_delay_range(float min_seconds, float max_seconds);
    std::pair<float, float> repeat_delay_range() const;

    void set_velocity(float vel);
    void set_velocity_range(float min_vel, float max_vel);
    std::pair<float, float> velocity_range() const;

    void set_duration(float seconds);
    void set_duration_range(float min_seconds, float max_seconds);
    std::pair<float, float> duration_range() const;

    std::vector<Particle> do_emit(double dt, uint32_t max_to_emit);

    ParticleSystem& system() { return system_; }

    void update(double dt);

    void activate();
    void deactivate();
    bool is_active() const { return is_active_; }

private:
    ParticleSystem& system_;
    ParticleEmitterType type_ = PARTICLE_EMITTER_POINT;
    kglt::Vec3 relative_position_ = kglt::Vec3();
    kglt::Vec3 direction_ = kglt::Vec3(1, 0, 0);
    kglt::Vec3 dimensions_ = kglt::Vec3(100, 100, 100);

    std::pair<float, float> duration_range_ = std::make_pair(0.0, 0.0);
    std::pair<float, float> repeat_delay_range_ = std::make_pair(0.0, 0.0);
    std::pair<float, float> velocity_range_ = std::make_pair(1.0, 1.0);
    std::pair<float, float> ttl_range_ = std::make_pair(5.0, 5.0);

    Degrees angle_ = Degrees(0);
    kglt::Colour colour_ = kglt::Colour::WHITE;
    int emission_rate_ = 10;

    float emission_accumulator_ = 0.0;

    float time_active_ = 0.0;
    float current_duration_ = 0.0;

    bool is_active_ = true;
};

typedef std::shared_ptr<ParticleEmitter> EmitterPtr;

class ParticleSystem :
    public virtual BoundableEntity,
    public Managed<ParticleSystem>,
    public generic::Identifiable<ParticleSystemID>,
    public ParentSetterMixin<MoveableObject>,
    public Source,
    public Loadable,
    public Renderable,
    public std::enable_shared_from_this<ParticleSystem> {

public:
    ParticleSystem(ParticleSystemID id, Stage* stage);
    ~ParticleSystem();

    void set_name(const unicode& name) { name_ = name; }
    const bool has_name() const { return !name_.empty(); }
    const unicode name() const { return name_; }

    void set_quota(int quota);
    int32_t quota() const { return quota_; }

    void set_particle_width(float width);
    float particle_width() const { return particle_width_; }

    void set_cull_each(bool val=true) { cull_each_ = val; }
    bool cull_each() const { return cull_each_; }

    int32_t emitter_count() const { return emitters_.size(); }
    EmitterPtr emitter(int32_t i) { return emitters_.at(i); }
    EmitterPtr push_emitter();
    void pop_emitter();

    //Boundable entity things
    const AABB aabb() const;
    const AABB transformed_aabb() const;

    unicode __unicode__() const { return name_; }

    void ask_owner_for_destruction();

    //Renderable stuff

    const MeshArrangement arrangement() const { return MESH_ARRANGEMENT_POINTS; }
    void set_render_priority(RenderPriority priority) { render_priority_ = priority; }
    virtual RenderPriority render_priority() const { return render_priority_; }
    virtual Mat4 final_transformation() const {
        return Mat4(); //Particles are absolutely positioned in the world
    }

    void set_material_id(MaterialID mat_id);
    virtual const MaterialID material_id() const { return material_id_; }
    virtual const bool is_visible() const { return MoveableObject::is_visible(); }

    void deactivate_emitters() { for(auto emitter: emitters_) { emitter->deactivate(); }; }
    void activate_emitters() { for(auto emitter: emitters_) { emitter->activate(); }; }

    void set_destroy_on_completion(bool value=true) { destroy_on_completion_ = value; }
    bool destroy_on_completion() const { return destroy_on_completion_; }

    bool has_repeating_emitters() const;
    bool has_active_emitters() const;

    void prepare_buffers() override;
    HardwareBuffer* vertex_attribute_buffer() const override {
        return vertex_buffer_.get();
    }

    HardwareBuffer* index_buffer() const override {
        return index_buffer_.get();
    }

    VertexSpecification vertex_attribute_specification() const {
        return vertex_data_->specification();
    }

    std::size_t index_element_count() const override {
        return index_data_->count();
    }

private:
    std::unique_ptr<HardwareBuffer> vertex_buffer_;
    std::unique_ptr<HardwareBuffer> index_buffer_;
    bool resize_buffers_ = false;

    bool vertex_buffer_dirty_ = false;
    bool index_buffer_dirty_ = false;

    inline VertexData* get_vertex_data() const {
        return vertex_data_;
    }

    inline IndexData* get_index_data() const {
        return index_data_;
    }

    unicode name_;
    int quota_ = 10;
    float particle_width_ = 100.0;
    bool cull_each_ = false;
    RenderPriority render_priority_ = RENDER_PRIORITY_MAIN;

    MaterialID material_id_;
    MaterialPtr material_ref_;

    std::vector<EmitterPtr> emitters_;
    std::list<Particle> particles_;

    void do_update(double dt);

    VertexData* vertex_data_ = nullptr;
    IndexData* index_data_ = nullptr;

    bool destroy_on_completion_ = false;
};

}

#endif // PARTICLES_H
