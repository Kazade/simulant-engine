#pragma once

#include <map>

#include "generic/managed.h"
#include "generic/identifiable.h"
#include "generic/property.h"
#include "nodes/stage_node.h"
#include "generic/manual_object.h"
#include "types.h"
#include "interfaces/nameable.h"
#include "viewport.h"

namespace smlt {

class Compositor;
class RenderableStore;

class Pipeline:
    public TypedDestroyableObject<Pipeline, Compositor>,
    public RefCounted<Pipeline> {

public:
    Pipeline(
        Compositor* render_sequence,
        const std::string& name,
        StagePtr stage, CameraPtr camera
    );

    virtual ~Pipeline();

    CameraPtr camera() const;
    StagePtr stage() const;
    TexturePtr target() const;
    uint32_t clear_flags() const;

    int32_t priority() const;
    PipelinePtr set_priority(int32_t priority);

    void deactivate();
    void activate();
    bool is_active() const { return is_active_; }

    PipelinePtr set_viewport(const Viewport& v) {
        viewport_ = v;
        return this;
    }

    PipelinePtr set_target(TextureID t) {
        target_ = t;
        return this;
    }

    PipelinePtr set_clear_flags(uint32_t viewport_clear_flags) {
        clear_mask_ = viewport_clear_flags;
        return this;
    }

    PipelinePtr set_detail_level_distances(
        float nearest_cutoff,
        float near_cutoff,
        float mid_cutoff,
        float far_cutoff
    );

    DetailLevel detail_level_at_distance(float dist) const;

    PipelinePtr set_name(const std::string& name) {
        name_ = name;
        return this;
    }

    std::string name() const {
        return name_;
    }

    PipelinePtr set_camera(CameraPtr c);

    /** Returns true if the pipeline has a valid stage and camera */
    bool is_complete() const {
        return stage_ && camera_;
    }
private:
    uint32_t id_ = 0;

    void set_stage(StagePtr s);

    sig::Connection stage_destroy_;
    sig::Connection camera_destroy_;

    Compositor* sequence_ = nullptr;
    int32_t priority_ = 0;
    StagePtr stage_;
    CameraPtr camera_;

    TextureID target_;
    Viewport viewport_;

    uint32_t clear_mask_ = 0;
    bool is_active_ = false;
    std::string name_;

    std::map<DetailLevel, float> detail_level_end_distances_;

    friend class Compositor;

public:
    Property<decltype(&Pipeline::viewport_)> viewport = { this, &Pipeline::viewport_ };
};

}
