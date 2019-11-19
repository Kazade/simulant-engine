#pragma once

#include <map>

#include "generic/managed.h"
#include "generic/identifiable.h"
#include "generic/property.h"
#include "nodes/stage_node.h"
#include "generic/manual_object.h"
#include "types.h"
#include "interfaces/nameable.h"

namespace smlt {

class RenderSequence;
class RenderableStore;

class Pipeline:
    public TypedDestroyableObject<Pipeline, RenderSequence>,
    public RefCounted<Pipeline>,
    public generic::Identifiable<PipelineID>,
    public Nameable {

public:
    Pipeline(PipelineID id,
        RenderSequence* render_sequence,
        StageID stage_id, CameraID camera_id);

    virtual ~Pipeline();

    CameraPtr camera() const;
    StagePtr stage() const;
    TexturePtr target() const;
    uint32_t clear_flags() const;

    int32_t priority() const;
    void set_priority(int32_t priority);

    void deactivate();
    void activate();
    bool is_active() const { return is_active_; }

    void set_viewport(const Viewport& v) { viewport_ = v; }
    void set_target(TextureID t) { target_ = t; }
    void set_clear_flags(uint32_t viewport_clear_flags) {
        clear_mask_ = viewport_clear_flags;
    }

    void set_detail_level_distances(
        float nearest_cutoff,
        float near_cutoff,
        float mid_cutoff,
        float far_cutoff
    );

    DetailLevel detail_level_at_distance(float dist) const;

    Property<Pipeline, Viewport> viewport = { this, &Pipeline::viewport_ };

private:
    void set_stage(StageID s);
    void set_camera(CameraID c) { camera_ = c; }

    RenderSequence* sequence_ = nullptr;
    int32_t priority_ = 0;
    StageID stage_;
    TextureID target_;
    CameraID camera_;
    Viewport viewport_;

    uint32_t clear_mask_ = 0;
    bool is_active_ = false;

    std::map<DetailLevel, float> detail_level_end_distances_;

    friend class RenderSequence;
};

}
