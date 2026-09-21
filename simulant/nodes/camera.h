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
    S_DEFINE_STAGE_NODE_META("camera");

    S_DEFINE_STAGE_NODE_PARAM(Camera, "projection_matrix", FloatArray, no_value,
                              "16 floats defining the projection matrix");

    Camera(Scene* owner);
    virtual ~Camera();

    /* Camera Proxies have no mass/body so their AABB is just 0,0,0, or their
     * position */
    const AABB& aabb() const override {
        return bounds_;
    }

    const AABB transformed_aabb() const override {
        // Zero-size box at the camera's world position - AABB's 2-arg
        // constructor takes (center, extents), so passing position() as
        // both would wrongly make its extents as large as the position
        // vector itself, rather than the zero-size point this is meant to
        // be (per aabb() above).
        return AABB(transform->position(), Vec3());
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
        update_frustum();
        return frustum_;
    }

    const Frustum& frustum() const {
        update_frustum();
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
        if(!clean_params<Camera>(params)) {
            return false;
        }

        set_projection_matrix(
            params.get<FloatArray>("projection_matrix").value());
        return StageNode::on_create(params);
    }

    // Editor-only wireframe frustum, built on demand (see
    // Application::is_editor_mode()) so shipped games pay zero memory cost
    // for it - see do_generate_renderables().
    void do_generate_renderables(batcher::RenderQueue* render_queue,
                                 const Camera* frustum_camera,
                                 const Viewport* viewport,
                                 const DetailLevel detail_level,
                                 Light** lights,
                                 const std::size_t light_count,
                                 bool respect_visibility = true) override;

private:
    AABB bounds_;
    mutable Frustum frustum_;
    mutable bool frustum_dirty_ = true;
    mutable uint32_t frustum_gen_ = UINT32_MAX;

    mutable Mat4 view_matrix_;
    mutable Mat4 projection_matrix_;

    void update_frustum() const;

    void on_transformation_changed() override;

    MeshPtr visualisation_mesh_;
    SubMeshPtr visualisation_submesh_ = nullptr;
    bool visualisation_dirty_ = true;

    void rebuild_visualisation_mesh();
};

class Camera2D: public Camera {
public:
    S_DEFINE_STAGE_NODE_META("camera2d");

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
    S_DEFINE_STAGE_NODE_META("camera3d");

    S_DEFINE_STAGE_NODE_PARAM(Camera3D, "znear", float, 1.0f,
                              "The camera near distance");
    S_DEFINE_STAGE_NODE_PARAM(Camera3D, "zfar", float, 100.0f,
                              "The camera far distance");
    S_DEFINE_STAGE_NODE_PARAM(Camera3D, "aspect", float, 1.0f, "Aspect ratio");
    S_DEFINE_STAGE_NODE_PARAM(Camera3D, "yfov", float, 60.0f,
                              "The camera field of view (in degrees)");

    Camera3D(Scene* owner) :
        Camera(owner) {}

    bool on_create(Params params) override;
};

} // namespace smlt
