#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "kazmath/mat4.h"
#include "generic/identifiable.h"
#include "generic/managed.h"

#include "object.h"
#include "frustum.h"
#include "renderer.h"
#include "sound.h"

namespace kglt {

class Camera :
    public Object,
    public generic::Identifiable<CameraID>,
    public Managed<Camera> {

public:
    Camera(Scene* scene, CameraID id);

    kmVec3 project_point(ViewportID vid, const kmVec3& point);
    void follow(ActorRef entity, const kglt::Vec3& offset);

    const kmMat4& view_matrix() { return view_matrix_; }
    const kmMat4& projection_matrix() const { return projection_matrix_; }

    Frustum& frustum() { return frustum_; }

    void set_perspective_projection(double fov, double aspect, double near=1.0, double far=1000.0f);
    void set_orthographic_projection(double left, double right, double bottom, double top, double near=-1.0, double far=1.0);
    double set_orthographic_projection_from_height(double desired_height_in_units, double ratio);

    void destroy();
private:
    Scene* scene_;

    Frustum frustum_;

    kmMat4 view_matrix_;
    kmMat4 projection_matrix_;

    ActorRef following_entity_;
    Vec3 following_offset_;

    void update_frustum();
    void transformation_changed() override { update_frustum(); }

    void do_update(double dt);

    bool can_set_parent(Object* parent) override {
        return false;
    }
};

}


#endif // CAMERA_H_INCLUDED
