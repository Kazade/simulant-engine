#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "object.h"
#include "object_visitor.h"
#include "frustum.h"
#include "renderer.h"

namespace kglt {

class Camera : public Object {
public:
    typedef std::tr1::shared_ptr<Camera> ptr;

	Camera():
		Object() {	
			
		kmQuaternionRotationYawPitchRoll(&rotation(), 180.0, 0.0, 0.0);
		kmQuaternionNormalize(&rotation(), &rotation());
	}

    void watch(Object& obj);
    void follow(Object& obj, float dist, float height=0.0f);
    void look_at(const Vec3& position);

    void accept(ObjectVisitor& visitor) {
        if(Renderer* renderer = dynamic_cast<Renderer*>(&visitor)) {
            //If the visitor is a renderer

            kmMat4* proj = &renderer->projection_stack().top();
            kmMat4 modelview;
            apply(&modelview); //Get the modelview transformations for this camera
            kmMat4 mvp;
            kmMat4Multiply(&mvp, proj, &modelview);
            frustum_.build(&mvp); //Update the frustum for this camera
        }

        do_accept<Camera>(this, visitor);
    }

    Frustum& frustum() { return frustum_; }

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

private:
    Frustum frustum_;
};

}


#endif // CAMERA_H_INCLUDED
