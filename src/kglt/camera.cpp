
#include "camera.h"

namespace kglt {

void Camera::rotate_x(float amount) {
    kmQuaternion rot;
    rot.x = 1.0f;
    rot.y = 0.0f;
    rot.z = 0.0f;
    rot.w = amount * kmPIOver180;
    kmQuaternionMultiply(&rotation(), &rotation(), &rot) ;
}



void Camera::rotate_z(float amount) {
    kmQuaternion rot;
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 1.0f;
    rot.w = amount * kmPIOver180;
    kmQuaternionMultiply(&rotation(), &rot, &rotation());
}


}
