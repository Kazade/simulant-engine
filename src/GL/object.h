#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>

#include "types.h"

namespace GL {

class ObjectVisitor;

class Object {
private:
    Vec3 position_;
//    Mat3 rotation_;

    Object* parent_;

protected:
    std::vector<Object*> children_;

    void attach_child(Object* child) {
        if(child->has_parent()) {
            child->parent().detach_child(child);
        }

        child->parent_ = this;
        children_.push_back(child);
    }

    void detach_child(Object* child) {
        //Make sure that the child has a parent
        if(!child->has_parent()) {
            return;
        }

        //Make sure *we* are the parent
        if(&child->parent() != this) {
            return;
        }

        //Erase the child from our children and set its parent to null
        child->parent_ = nullptr;
        children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
    }

public:
    Object():
        parent_(nullptr) {

    }

    virtual ~Object();

    virtual void move(float x, float y, float z);

    void set_parent(Object* p) {
        if(has_parent()) {
            parent().detach_child(this);
        }

        if(p) {
            p->attach_child(this);
        }

    }

    bool has_parent() const { return parent_ != nullptr; }
    Object& parent() { assert(parent_); return *parent_; }
    Object& child(uint32_t i);

    Vec3& position() { return position_; }

    template<typename T>
    Object* root_as() {
        Object* self = this;
        while(self->has_parent()) {
            self = &self->parent();
        }

        return dynamic_cast<T>(self);
    }

    virtual void accept(ObjectVisitor& visitor) = 0;
};

}

#endif // OBJECT_H_INCLUDED
