#include "window.h"
#include "stage.h"
#include "compositor.h"
#include "layer.h"
#include "nodes/camera.h"

namespace smlt {

static uint32_t PIPELINE_COUNTER = 0;

Layer::Layer(Compositor* render_sequence,
    StageNode* subtree, CameraPtr camera):
        TypedDestroyableObject<Layer, Compositor>(render_sequence),
        id_(++PIPELINE_COUNTER),
        sequence_(render_sequence),
        priority_(0),
        is_active_(false) {

    set_stage_node(subtree);
    set_camera(camera);

    /* Set sane defaults for detail ranges */
    detail_level_end_distances_[DETAIL_LEVEL_NEAREST] = 25.0f;
    detail_level_end_distances_[DETAIL_LEVEL_NEAR] = 50.0f;
    detail_level_end_distances_[DETAIL_LEVEL_MID] = 100.0f;
    detail_level_end_distances_[DETAIL_LEVEL_FAR] = 200.0f;
    detail_level_end_distances_[DETAIL_LEVEL_FARTHEST] = 400.0f;
}

LayerPtr Layer::set_detail_level_distances(float nearest_cutoff,
    float near_cutoff, float mid_cutoff, float far_cutoff) {

    detail_level_end_distances_[DETAIL_LEVEL_NEAREST] = nearest_cutoff;
    detail_level_end_distances_[DETAIL_LEVEL_NEAR] = near_cutoff;
    detail_level_end_distances_[DETAIL_LEVEL_MID] = mid_cutoff;
    detail_level_end_distances_[DETAIL_LEVEL_FAR] = far_cutoff;

    return this;
}

DetailLevel Layer::detail_level_at_distance(float dist) const {
    /*
     * Given a distance (e.g. from a camera), this will return the detail level
     * that should be used at that distance
     */
    if(dist < detail_level_end_distances_[DETAIL_LEVEL_NEAREST]) return DETAIL_LEVEL_NEAREST;
    if(dist < detail_level_end_distances_[DETAIL_LEVEL_NEAR]) return DETAIL_LEVEL_NEAR;
    if(dist < detail_level_end_distances_[DETAIL_LEVEL_MID]) return DETAIL_LEVEL_MID;
    if(dist < detail_level_end_distances_[DETAIL_LEVEL_FAR]) return DETAIL_LEVEL_FAR;

    return DETAIL_LEVEL_FARTHEST;
}

LayerPtr Layer::set_camera(CameraPtr c) {
    camera_destroy_.disconnect();
    camera_ = c;
    if(camera_) {
        camera_destroy_ = camera_->signal_destroyed().connect([&]() {
            camera_ = nullptr;
            camera_destroy_.disconnect();
        });
    }
    return this;
}

LayerPtr Layer::set_priority(int32_t priority) {
    if(priority_ != priority) {
        priority_ = priority;

        /* If the priority changed, we need to update the render sequence */
        sequence_->sort_layers();
    }

    return this;
}

Layer::~Layer() {
    deactivate();

    camera_destroy_.disconnect();
    stage_destroy_.disconnect();
}

CameraPtr Layer::camera() const {
    return camera_;
}

StageNode* Layer::stage_node() const {
    return node_;
}

TexturePtr Layer::target() const {
    return target_;
}

uint32_t Layer::clear_flags() const {
    return clear_mask_;
}

int32_t Layer::priority() const {
    return priority_;
}

void Layer::deactivate() {
    if(!is_active_) return;

    is_active_ = false;

    if(node_) {
        node_->active_pipeline_count_--;
    }
}

void Layer::activate() {
    if(is_active_) return;

    is_active_ = true;

    if(node_) {
        node_->active_pipeline_count_++;
    }
}

void Layer::set_stage_node(StageNode* node) {
    if(node_ && is_active()) {
        node_->active_pipeline_count_--;
    }

    stage_destroy_.disconnect();

    node_ = node;

    if(node_) {
        stage_destroy_ = node_->signal_destroyed().connect([&]() {
            node_ = nullptr;
            stage_destroy_.disconnect();
        });

        if(is_active()) {
            node_->active_pipeline_count_++;
        }
    }
}

}
