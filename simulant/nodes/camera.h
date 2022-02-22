#pragma once

#include "stage_node.h"
#include "../generic/identifiable.h"
#include "../generic/optional.h"
#include "../generic/manual_object.h"
#include "../math/aabb.h"
#include "../frustum.h"
#include "../sound.h"

namespace smlt {

class RenderTarget;

class Camera:
    public TypedDestroyableObject<Camera, Stage>,
    public ContainerNode,
    public generic::Identifiable<CameraID>,
    public ChainNameable<Camera>,
    public AudioSource,
    public RefCounted<Camera> {

public:
    using ContainerNode::_get_renderables;

    Camera(Stage* stage, SoundDriver* sound_driver);
    virtual ~Camera();

    /* Camera Proxies have no mass/body so their AABB is just 0,0,0, or their position */
    const AABB& aabb() const override {
        return bounds_;
    }

    const AABB transformed_aabb() const override {
        return AABB(position(), position());
    }

    void clean_up() override {
        StageNode::clean_up();
    }

    void update(float dt) override;

    // Converts an OpenGL unit to window space
    smlt::optional<Vec3> project_point(const RenderTarget& target, const Viewport& viewport, const Vec3& point) const;

    // Converts a pixel to OpenGL units (z-input should be read from the depth buffer)
    smlt::optional<Vec3> unproject_point(const RenderTarget& target, const Viewport& viewport, const Vec3& win_point);

    const Mat4& view_matrix() const { return view_matrix_; }
    const Mat4& projection_matrix() const { return projection_matrix_; }

    Frustum& frustum() { return frustum_; }

    void set_perspective_projection(const Degrees &fov, float aspect, float near=1.0f, float far=1000.0f);
    void set_orthographic_projection(float left, float right, float bottom, float top, float near=-1.0, float far=1.0);
    float set_orthographic_projection_from_height(float desired_height_in_units, float ratio);

private:
    AABB bounds_;
    Frustum frustum_;

    smlt::Mat4 transform_;
    Mat4 view_matrix_;
    Mat4 projection_matrix_;

    void update_frustum();

    void update_transformation_from_parent() override;
};

}
