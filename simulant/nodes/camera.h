#pragma once

#include "stage_node.h"
#include "../generic/identifiable.h"
#include "../generic/optional.h"
#include "../math/aabb.h"
#include "../frustum.h"

namespace smlt {

class RenderTarget;

class Camera:
    public StageNode,
    public generic::Identifiable<CameraID>,
    public Managed<Camera> {

public:
    Camera(CameraID camera_id, Stage* stage);
    ~Camera();

    void ask_owner_for_destruction();

    /* Camera Proxies have no mass/body so their AABB is just 0,0,0, or their position */
    const AABB& aabb() const {
        return bounds_;
    }

    const AABB transformed_aabb() const {
        return AABB(position(), position());
    }

    void cleanup() override {
        StageNode::cleanup();
    }

    // Converts an OpenGL unit to window space
    smlt::optional<Vec3> project_point(const RenderTarget& target, const Viewport& viewport, const Vec3& point) const;

    // Converts a pixel to OpenGL units (z-input should be read from the depth buffer)
    smlt::optional<Vec3> unproject_point(const RenderTarget& target, const Viewport& viewport, const Vec3& win_point);

    const Mat4& view_matrix() const { return view_matrix_; }
    const Mat4& projection_matrix() const { return projection_matrix_; }

    Frustum& frustum() { return frustum_; }

    void set_perspective_projection(const Degrees &fov, double aspect, double near=1.0, double far=1000.0f);
    void set_orthographic_projection(double left, double right, double bottom, double top, double near=-1.0, double far=1.0);
    double set_orthographic_projection_from_height(double desired_height_in_units, double ratio);

private:    
    AABB bounds_;
    Frustum frustum_;

    smlt::Mat4 transform_;
    Mat4 view_matrix_;
    Mat4 projection_matrix_;

    void update_frustum();

    void on_position_set(const Vec3& oldp, const Vec3& newp) override;
    void on_rotation_set(const Quaternion& oldr, const Quaternion& newr) override;
    void on_scaling_set(const Vec3& olds, const Vec3& news) override;
    void on_parent_set(TreeNode* oldp, TreeNode* newp) override;

    void update_position_from_parent(bool _recalc_bounds);
    void update_rotation_from_parent();
};

}
