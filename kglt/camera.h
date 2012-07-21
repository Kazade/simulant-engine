#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "object.h"
#include "object_visitor.h"

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
        do_accept<Camera>(this, visitor);
    }
};

}


#endif // CAMERA_H_INCLUDED
