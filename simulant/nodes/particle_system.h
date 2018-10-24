#pragma once

#include <memory>
#include <unordered_map>

#include "stage_node.h"

#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../renderers/renderer.h"
#include "../sound.h"
#include "../interfaces.h"
#include "../types.h"
#include "../vertex_data.h"

#include "particles/emitter.h"
#include "particles/manipulator.h"

namespace smlt {


class ParticleSystem;

typedef sig::signal<void (ParticleSystem*, MaterialID, MaterialID)> ParticleSystemMaterialChangedSignal;

class ParticleSystem :
    public StageNode,
    public Managed<ParticleSystem>,
    public generic::Identifiable<ParticleSystemID>,
    public Source,
    public Loadable,
    public HasMutableRenderPriority,
    public Renderable {

    DEFINE_SIGNAL(ParticleSystemMaterialChangedSignal, signal_material_changed);

public:
    ParticleSystem(ParticleSystemID id, Stage* stage, SoundDriver *sound_driver);
    ~ParticleSystem();

    const AABB& aabb() const override;
    const AABB transformed_aabb() const override {
        return StageNode::transformed_aabb();
    }

    void set_quota(std::size_t quota);
    int32_t quota() const { return quota_; }

    void set_particle_width(float width);
    float particle_width() const { return particle_width_; }

    void set_particle_height(float height);
    float particle_height() const { return particle_height_; }

    void set_cull_each(bool val=true) { cull_each_ = val; }
    bool cull_each() const { return cull_each_; }

    int32_t emitter_count() const { return emitters_.size(); }
    particles::EmitterPtr emitter(int32_t i) { return emitters_.at(i); }
    particles::EmitterPtr push_emitter();
    void pop_emitter();

    void ask_owner_for_destruction() override;

    //Renderable stuff

    const MeshArrangement arrangement() const override { return MESH_ARRANGEMENT_TRIANGLES; }
    virtual Mat4 final_transformation() const override {
        return Mat4(); //Particles are absolutely positioned in the world
    }

    void set_material_id(MaterialID mat_id);
    virtual const MaterialID material_id() const override { return material_id_; }

    void deactivate_emitters() { for(auto emitter: emitters_) { emitter->deactivate(); }; }
    void activate_emitters() { for(auto emitter: emitters_) { emitter->activate(); }; }

    void set_destroy_on_completion(bool value=true) { destroy_on_completion_ = value; }
    bool destroy_on_completion() const { return destroy_on_completion_; }

    bool has_repeating_emitters() const;
    bool has_active_emitters() const;

    void prepare_buffers(Renderer* renderer) override;
    HardwareBuffer* vertex_attribute_buffer() const override {
        return vertex_buffer_.get();
    }

    HardwareBuffer* index_buffer() const override {
        return index_buffer_.get();
    }

    VertexSpecification vertex_attribute_specification() const override {
        return vertex_data_->specification();
    }

    std::size_t index_element_count() const override {
        return index_data_->count();
    }

    IndexType index_type() const override {
        return index_data_->index_type();
    }

    RenderPriority render_priority() const override {
        return HasMutableRenderPriority::render_priority();
    }

    const bool is_visible() const override {
        return StageNode::is_visible();
    }

    void cleanup() override {
        StageNode::cleanup();
    }

    template<typename M, typename... Args>
    particles::Manipulator* new_manipulator(Args&& ...args) {
        auto m = std::make_shared<M>(std::forward<Args>(args)...);
        manipulators_.push_back(m);
        return m.get();
    }

    RenderableList _get_renderables(const Frustum &frustum, DetailLevel detail_level) const {
        auto ret = RenderableList();
        std::shared_ptr<Renderable> sptr = std::const_pointer_cast<ParticleSystem>(shared_from_this());
        ret.push_back(sptr);
        return ret;
    }

private:
    const static int32_t INITIAL_QUOTA = 10;

    AABB aabb_;
    void calc_aabb();

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

    std::string name_;
    std::size_t quota_ = 0;
    float particle_width_ = 100.0f;
    float particle_height_ = 100.0f;
    bool cull_each_ = false;

    MaterialID material_id_;
    MaterialPtr material_ref_;

    std::vector<particles::EmitterPtr> emitters_;
    std::vector<particles::Particle> particles_;
    std::vector<particles::ManipulatorPtr> manipulators_;

    void update(float dt) override;

    VertexData* vertex_data_ = nullptr;
    IndexData* index_data_ = nullptr;

    bool destroy_on_completion_ = false;
};

}
