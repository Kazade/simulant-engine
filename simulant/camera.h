#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "deps/kazmath/mat4.h"
#include "generic/identifiable.h"
#include "generic/managed.h"

#include "nodes/stage_node.h"
#include "frustum.h"
#include "renderers/renderer.h"
#include "sound.h"

namespace smlt {

class Camera:
    public generic::Identifiable<CameraID>,
    public Managed<Camera> {

public:
    Camera(CameraID id, WindowBase* window);

    kmVec3 project_point(const RenderTarget& target, const Viewport& viewport, const kmVec3& point);

    const Mat4& view_matrix() { return view_matrix_; }
    const Mat4& projection_matrix() const { return projection_matrix_; }

    Frustum& frustum() { return frustum_; }

    void set_perspective_projection(double fov, double aspect, double near=1.0, double far=1000.0f);
    void set_orthographic_projection(double left, double right, double bottom, double top, double near=-1.0, double far=1.0);
    double set_orthographic_projection_from_height(double desired_height_in_units, double ratio);

    void set_transform(const smlt::Mat4& transform);

    bool has_proxy() const { return bool(proxy_); }
    void set_proxy(CameraProxy* proxy) { proxy_ = proxy; }

    CameraProxy& proxy() {
        assert(proxy_);
        return *proxy_;
    }

    const Mat4& transform() const { return transform_; }
private:
    WindowBase* window_;
    CameraProxy* proxy_;

    Frustum frustum_;

    smlt::Mat4 transform_;
    Mat4 view_matrix_;
    Mat4 projection_matrix_;

    void update_frustum();
};

}


#endif // CAMERA_H_INCLUDED
