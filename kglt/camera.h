#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "object.h"
#include "frustum.h"
#include "renderer.h"

namespace kglt {

class Camera :
    public Object,
    public generic::VisitableBase<Camera> {
public:
    typedef std::tr1::shared_ptr<Camera> ptr;

    Camera();

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

private:
    Frustum frustum_;
    kmMat4 projection_matrix_;

    void update_frustum() {
        kmMat4 modelview;
        apply(&modelview); //Get the modelview transformations for this camera
        kmMat4 mvp;
        kmMat4Multiply(&mvp, &projection_matrix_, &modelview);
        frustum_.build(&mvp); //Update the frustum for this camera
    }
};

}


#endif // CAMERA_H_INCLUDED
