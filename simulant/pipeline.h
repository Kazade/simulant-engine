#pragma once

#include <map>

#include "generic/managed.h"
#include "generic/identifiable.h"
#include "generic/property.h"
#include "nodes/stage_node.h"
#include "generic/manual_object.h"
#include "types.h"

namespace smlt {

class RenderSequence;
class RenderableStore;

class Pipeline:
    public TypedDestroyableObject<Pipeline, RenderSequence>,
    public RefCounted<Pipeline>,
    public generic::Identifiable<PipelineID> {

public:
    Pipeline(
        PipelineID id,
        RenderSequence* render_sequence
    );

    virtual ~Pipeline();

    CameraID camera_id() { return camera_; }
    StageID stage_id() { return stage_; }
    TextureID target_id() { return target_; }
    uint32_t clear_flags() const { return clear_mask_; }

    int32_t priority() const { return priority_; }
    void set_priority(int32_t priority);

    void deactivate();
    void activate();
    bool is_active() const { return is_active_; }

    void set_stage(StageID s);
    void set_camera(CameraID c) { camera_ = c; }
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
    RenderSequence* sequence_;
    int32_t priority_;
    StageID stage_;
    TextureID target_;
    CameraID camera_;
    Viewport viewport_;

    uint32_t clear_mask_ = 0;

    bool is_active_;

    std::map<DetailLevel, float> detail_level_end_distances_;

    friend class RenderSequence;
};

}
