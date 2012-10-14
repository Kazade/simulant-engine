#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "kazmath/mat4.h"
#include "generic/identifiable.h"

#include "object.h"
#include "frustum.h"
#include "renderer.h"

namespace kglt {

class Camera :
    public Object,
    public generic::Identifiable<CameraID> {
public:
    VIS_DEFINE_VISITABLE();

    typedef std::tr1::shared_ptr<Camera> ptr;

    Camera(Scene* scene, CameraID id);

    void watch(Object& obj);
    void follow(Object& obj, float dist, float height=0.0f);
    void look_at(const Vec3& position);

    void apply(kmMat4* modelview_out) {
        kmVec3& pos = position();
        kmQuaternion& rot = rotation();
        kmMat4 rot_mat;
        kmMat4RotationQuaternion(&rot_mat, &rot);

        rot_mat.mat[12] = pos.x;
        rot_mat.mat[13] = pos.y;
        rot_mat.mat[14] = pos.z;

        kmVec3 up;
        kmVec3 forward;
        kmVec3 centre;

        kmMat4GetForwardVec3(&forward, &rot_mat);

        kmMat4GetUpVec3(&up, &rot_mat);
        kmVec3Add(&centre, &pos, &forward);

        kmMat4Identity(modelview_out);
        kmMat4LookAt(modelview_out, &pos, &centre, &up);
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

private:
    Frustum frustum_;
    kmMat4 projection_matrix_;
    kmMat4 modelview_matrix_;

    void update_frustum() {
        apply(&modelview_matrix_); //Get the modelview transformations for this camera
        kmMat4 mvp;
        kmMat4Multiply(&mvp, &projection_matrix_, &modelview_matrix_);
        frustum_.build(&mvp); //Update the frustum for this camera
    }
};

}


#endif // CAMERA_H_INCLUDED
