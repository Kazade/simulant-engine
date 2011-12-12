#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "object.h"
#include "object_visitor.h"

namespace GL {

class Camera : public Object {
public:
    void accept(ObjectVisitor& visitor) {
        for(Object* child: children_) {
            child->accept(visitor);
        }

        visitor.visit(this);
    }
};

}


#endif // CAMERA_H_INCLUDED
