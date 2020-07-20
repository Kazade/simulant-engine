#include "core.h"
#include "stage.h"
#include "compositor.h"
#include "pipeline.h"

namespace smlt {

Pipeline::Pipeline(Compositor* render_sequence,
    const std::string &name, StageID stage_id, CameraID camera_id):
        TypedDestroyableObject<Pipeline, Compositor>(render_sequence),
        sequence_(render_sequence),
        priority_(0),
        is_active_(false) {

    if(name.empty()) {
        throw std::logic_error("You must specify a name for a pipeline");
    }

    set_name(name);
    set_stage(stage_id);
    set_camera(camera_id);

    /* Set sane defaults for detail ranges */
    detail_level_end_distances_[DETAIL_LEVEL_NEAREST] = 25.0f;
    detail_level_end_distances_[DETAIL_LEVEL_NEAR] = 50.0f;
    detail_level_end_distances_[DETAIL_LEVEL_MID] = 100.0f;
    detail_level_end_distances_[DETAIL_LEVEL_FAR] = 200.0f;
    detail_level_end_distances_[DETAIL_LEVEL_FARTHEST] = 400.0f;
}

PipelinePtr Pipeline::set_detail_level_distances(float nearest_cutoff,
    float near_cutoff, float mid_cutoff, float far_cutoff) {

    detail_level_end_distances_[DETAIL_LEVEL_NEAREST] = nearest_cutoff;
    detail_level_end_distances_[DETAIL_LEVEL_NEAR] = near_cutoff;
    detail_level_end_distances_[DETAIL_LEVEL_MID] = mid_cutoff;
    detail_level_end_distances_[DETAIL_LEVEL_FAR] = far_cutoff;

    return this;
}

DetailLevel Pipeline::detail_level_at_distance(float dist) const {
    /*
     * Given a distance (e.g. from a camera), this will return the detail level
     * that should be used at that distance
     */
    if(dist < detail_level_end_distances_.at(DETAIL_LEVEL_NEAREST)) return DETAIL_LEVEL_NEAREST;
    if(dist < detail_level_end_distances_.at(DETAIL_LEVEL_NEAR)) return DETAIL_LEVEL_NEAR;
    if(dist < detail_level_end_distances_.at(DETAIL_LEVEL_MID)) return DETAIL_LEVEL_MID;
    if(dist < detail_level_end_distances_.at(DETAIL_LEVEL_FAR)) return DETAIL_LEVEL_FAR;

    return DETAIL_LEVEL_FARTHEST;
}


PipelinePtr Pipeline::set_priority(int32_t priority) {
    if(priority_ != priority) {
        priority_ = priority;

        /* If the priority changed, we need to update the render sequence */
        sequence_->sort_pipelines();
    }

    return this;
}

Pipeline::~Pipeline() {
    deactivate();
}

CameraPtr Pipeline::camera() const {
    return stage()->camera(camera_);
}

StagePtr Pipeline::stage() const {
    return sequence_->window->stage(stage_);
}

TexturePtr Pipeline::target() const {
    return stage()->assets->texture(target_);
}

uint32_t Pipeline::clear_flags() const {
    return clear_mask_;
}

int32_t Pipeline::priority() const {
    return priority_;
}

void Pipeline::deactivate() {
    if(!is_active_) return;

    is_active_ = false;

    if(stage_) {
        auto s = sequence_->window->stage(stage_);
        if(s) {
            s->active_pipeline_count_--;
        }
    }
}

void Pipeline::activate() {
    if(is_active_) return;

    is_active_ = true;

    if(stage_) {
        auto s = sequence_->window->stage(stage_);
        if(s) {
            s->active_pipeline_count_++;
        }
    }
}

void Pipeline::set_stage(StageID s) {
    if(stage_ && is_active()) {
        StagePtr s = sequence_->window->stage(stage_);
        s->active_pipeline_count_--;
    }

    stage_ = s;

    if(stage_&& is_active()) {
        StagePtr s = sequence_->window->stage(stage_);
        if(s) {
            s->active_pipeline_count_++;
        }
    }
}

}
