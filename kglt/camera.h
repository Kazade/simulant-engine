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
    public Managed<Camera>,
    public Source {
public:
    Camera(SubScene* subscene, CameraID id);

    kmVec3 project_point(ViewportID vid, const kmVec3& point);

    void watch(Object& obj);
    void follow(EntityID entity, const kglt::Vec3& offset);

    void look_at(const Vec3& position);

    kmMat4 view_matrix() {
        kmMat4 transform = this->absolute_transformation();
        kmMat4Inverse(&transform, &transform);

        return transform;
    }

    void apply(kmMat4* modelview_out) {


        kmMat4 rot_mat = absolute_transformation();

        kmVec3 up;
        kmVec3 forward;
        kmVec3 centre;

        kmMat4GetForwardVec3RH(&forward, &rot_mat);

        kmMat4GetUpVec3(&up, &rot_mat);
        kmVec3Add(&centre, &absolute_position(), &forward);

        kmMat4Identity(modelview_out);
        kmMat4LookAt(modelview_out, &absolute_position(), &centre, &up);
    }

    Frustum& frustum() { return frustum_; }

    void set_perspective_projection(double fov, double aspect, double near=1.0, double far=1000.0f);
    void set_orthographic_projection(double left, double right, double bottom, double top, double near=-1.0, double far=1.0);
    double set_orthographic_projection_from_height(double desired_height_in_units, double ratio);

    const kmMat4& projection_matrix() const { return projection_matrix_; }
    const kmMat4& modelview_matrix() {
        apply(&modelview_matrix_); //Get the modelview transformations for this camera
        return modelview_matrix_;

    }

    void destroy();
private:
    Frustum frustum_;
    kmMat4 projection_matrix_;
    kmMat4 modelview_matrix_;

    EntityID following_entity_;
    Vec3 following_offset_;

    void update_frustum() {
        apply(&modelview_matrix_); //Get the modelview transformations for this camera
        kmMat4 mvp;
        kmMat4Multiply(&mvp, &projection_matrix_, &modelview_matrix_);
        frustum_.build(&mvp); //Update the frustum for this camera
    }

    void transformation_changed() override { update_frustum(); }

    void do_update(double dt);
};

}


#endif // CAMERA_H_INCLUDED
