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

enum LayerActivationMode {
    LAYER_ACTIVATION_MODE_AUTOMATIC,
    LAYER_ACTIVATION_MODE_MANUAL
};

class Layer:
    public TypedDestroyableObject<Layer, Compositor>,
    public RefCounted<Layer> {

public:
    Layer(Compositor* render_sequence,
        StageNode* subtree, CameraPtr camera
    );

    virtual ~Layer();

    CameraPtr camera() const;
    StageNode* stage_node() const;
    TexturePtr target() const;
    uint32_t clear_flags() const;

    int32_t priority() const;
    LayerPtr set_priority(int32_t priority);

    void deactivate();
    void activate();
    bool is_active() const { return is_active_; }

    LayerPtr set_viewport(const Viewport& v) {
        viewport_ = v;
        return this;
    }

    LayerPtr set_target(TexturePtr t) {
        target_ = t;
        return this;
    }

    LayerPtr set_clear_flags(uint32_t viewport_clear_flags) {
        clear_mask_ = viewport_clear_flags;
        return this;
    }

    LayerPtr set_detail_level_distances(
        float nearest_cutoff,
        float near_cutoff,
        float mid_cutoff,
        float far_cutoff
    );

    DetailLevel detail_level_at_distance(float dist) const;

    LayerPtr set_name(const std::string& name) {
        name_ = name;
        return this;
    }

    std::string name() const {
        return name_;
    }

    LayerPtr set_camera(CameraPtr c);

    /** Returns true if the pipeline has a valid stage and camera */
    bool is_complete() const {
        return node_ && camera_;
    }

    void set_activation_mode(LayerActivationMode mode) {
        mode_ = mode;
    }

    LayerActivationMode activation_mode() const {
        return mode_;
    }

    uint32_t id() const {
        return id_;
    }

private:
    uint32_t id_ = 0;

    void set_stage_node(StageNode* s);

    sig::Connection stage_destroy_;
    sig::Connection camera_destroy_;

    Compositor* sequence_ = nullptr;
    int32_t priority_ = 0;
    StageNodePtr node_ = nullptr;
    CameraPtr camera_;

    TexturePtr target_;
    Viewport viewport_;

    uint32_t clear_mask_ = 0;
    bool is_active_ = false;
    std::string name_;

    float detail_level_end_distances_[DETAIL_LEVEL_MAX];

    LayerActivationMode mode_ = LAYER_ACTIVATION_MODE_AUTOMATIC;

    friend class Compositor;

public:
    Property<decltype(&Layer::viewport_)> viewport = { this, &Layer::viewport_ };
};

}
