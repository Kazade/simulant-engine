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
	}

    void watch(Object& obj);
    void follow(Object& obj, float dist, float height=0.0f);
    void look_at(const Vec3& position);

    void accept(ObjectVisitor& visitor) {
        for(Object* child: children_) {
            child->accept(visitor);
        }

        visitor.visit(this);
    }
};

}


#endif // CAMERA_H_INCLUDED
