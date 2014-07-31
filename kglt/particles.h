#ifndef PARTICLES_H
#define PARTICLES_H

#include <memory>
#include <unordered_map>

#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/protected_ptr.h"

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
    public Nameable,
    public Managed<ParticleSystem>,
    public generic::Identifiable<ParticleSystemID>,
    public ParentSetterMixin<Object>,
    public Source,
    public Protectable,
    public Loadable,
    public Renderable {

public:
    ParticleSystem(Stage* stage, ParticleSystemID id);

    void set_name(const unicode& name) { name_ = name; }
    const bool has_name() const { return !name_.empty(); }
    const unicode name() const { return name_; }

    void set_quota(int quota) {
        quota_ = quota;
    }

    int32_t quota() const { return quota_; }

    void set_particle_width(float width) { particle_width_ = width; }
    float particle_width() const { return particle_width_; }

    void set_particle_height(float height) { particle_height_ = height; }
    float particle_height() const { return particle_height_; }

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

    virtual void _update_vertex_array_object();
    virtual void _bind_vertex_array_object();

    void set_render_priority(RenderPriority priority) { render_priority_ = priority; }
    virtual RenderPriority render_priority() const { return render_priority_; }
    virtual Mat4 final_transformation() const {
        return Mat4(); //Particles are absolutely positioned in the world
    }

    void set_material_id(MaterialID mat_id);
    virtual const MaterialID material_id() const { return material_id_; }
    virtual const bool is_visible() const { return Object::is_visible(); }

    virtual MeshID instanced_mesh_id() const { return MeshID(); } //We don't support instancing
    virtual SubMeshIndex instanced_submesh_id() const { return 0; } //We don't support instancing

    const VertexData& vertex_data() const { return vertex_data_; }
    const IndexData& index_data() const { return index_data_; }

    WindowBase& window();

    void deactivate_emitters() { for(auto emitter: emitters_) { emitter->deactivate(); }; }
    void activate_emitters() { for(auto emitter: emitters_) { emitter->activate(); }; }

private:
    unicode name_;
    int quota_ = 10;
    float particle_width_ = 100.0;
    float particle_height_ = 100.0;
    bool cull_each_ = false;
    RenderPriority render_priority_ = RENDER_PRIORITY_MAIN;

    MaterialID material_id_;
    MaterialPtr material_ref_;

    std::vector<EmitterPtr> emitters_;
    std::list<Particle> particles_;

    void do_update(double dt);

    VertexData vertex_data_;
    IndexData index_data_;

    VertexArrayObject vao_;
};

}

#endif // PARTICLES_H
