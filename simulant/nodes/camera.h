#pragma once

#include "../frustum.h"
#include "../generic/identifiable.h"
#include "../generic/manual_object.h"
#include "../generic/optional.h"
#include "../math/aabb.h"
#include "../sound.h"
#include "simulant/utils/params.h"
#include "stage_node.h"

namespace smlt {

class RenderTarget;
class CameraParams {};

class Camera: public ContainerNode, public ChainNameable<Camera> {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_CAMERA, "camera");

    S_DEFINE_STAGE_NODE_PARAM(Camera, "projection_matrix", FloatArray, no_value,
                              "16 floats defining the projection matrix");

    using ContainerNode::do_generate_renderables;

    Camera(Scene* owner);
    virtual ~Camera();

    /* Camera Proxies have no mass/body so their AABB is just 0,0,0, or their
     * position */
    const AABB& aabb() const override {
        return bounds_;
    }

    const AABB transformed_aabb() const override {
        return AABB(transform->position(), transform->position());
    }

    // Converts an OpenGL unit to window space
    smlt::optional<Vec3> project_point(const RenderTarget& target,
                                       const Viewport& viewport,
                                       const Vec3& point) const;

    // Converts a pixel to OpenGL units (z-input should be read from the depth
    // buffer)
    smlt::optional<Vec3> unproject_point(const RenderTarget& target,
                                         const Viewport& viewport,
                                         const Vec3& win_point);

    const Mat4& view_matrix() const {
        return view_matrix_;
    }

    const Mat4& projection_matrix() const {
        return projection_matrix_;
    }

    Frustum& frustum() {
        return frustum_;
    }
    const Frustum& frustum() const {
        return frustum_;
    }

    void set_projection_matrix(const Mat4& matrix);
    void set_perspective_projection(const Degrees& fov, float aspect,
                                    float near = 1.0f, float far = 1000.0f);
    void set_orthographic_projection(float left, float right, float bottom,
                                     float top, float near = -1.0,
                                     float far = 1.0);
    float set_orthographic_projection_from_height(float desired_height_in_units,
                                                  float ratio);

    bool on_create(Params params) override {
        _S_UNUSED(params);
        return true;
    }

private:
    AABB bounds_;
    Frustum frustum_;

    Mat4 view_matrix_;
    Mat4 projection_matrix_;

    void update_frustum();

    void on_transformation_changed() override;
};

class Camera2D: public Camera {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_CAMERA2D, "camera2d");

    S_DEFINE_STAGE_NODE_PARAM(Camera2D, "xmag", float, 1.0f,
                              "Width of the view");
    S_DEFINE_STAGE_NODE_PARAM(Camera2D, "ymag", float, 1.0f,
                              "Height of the view");
    S_DEFINE_STAGE_NODE_PARAM(Camera2D, "znear", float, 1.0f,
                              "The camera near distance");
    S_DEFINE_STAGE_NODE_PARAM(Camera2D, "zfar", float, 100.0f,
                              "The camera far distance");

    Camera2D(Scene* owner) :
        Camera(owner) {}

    bool on_create(Params params) override;
};

class Camera3D: public Camera {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_CAMERA3D, "camera3d");

    S_DEFINE_STAGE_NODE_PARAM(Camera3D, "znear", float, 1.0f,
                              "The camera near distance");
    S_DEFINE_STAGE_NODE_PARAM(Camera3D, "zfar", float, 100.0f,
                              "The camera far distance");
    S_DEFINE_STAGE_NODE_PARAM(Camera3D, "aspect_ratio", float, 1.0f,
                              "Aspect ratio");
    S_DEFINE_STAGE_NODE_PARAM(Camera3D, "yfov", float, 60.0f,
                              "The camera field of view (in degrees)");

    Camera3D(Scene* owner) :
        Camera(owner) {}

    bool on_create(Params params) override;
};

} // namespace smlt
