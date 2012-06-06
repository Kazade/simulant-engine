#ifndef KGLT_OBJECT_H_INCLUDED
#define KGLT_OBJECT_H_INCLUDED

#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <tr1/memory>

#include "kazmath/vec3.h"
#include "kazmath/quaternion.h"
#include "types.h"

namespace kglt {

class ObjectVisitor;
class Scene;

class Object {
private:
    static uint64_t object_counter;
    uint64_t id_;

    kmVec3 position_;
    kmQuaternion rotation_;

protected:
    Object* parent_;

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

	virtual void do_update(double dt) {}

    void* user_data_;
    bool is_visible_;
public:
    typedef std::tr1::shared_ptr<Object> ptr;

    Object():
        id_(++object_counter),
        parent_(nullptr),
        user_data_(nullptr),
        is_visible_(true) {

        kmVec3Fill(&position_, 0.0, 0.0, 0.0);
        kmQuaternionIdentity(&rotation_);
        
		//rotate_y(180.0);
    }

    virtual ~Object();

	virtual void update(double dt) {
		do_update(dt);
		
		for(Object* child: children_) {
			child->update(dt);
		}
	}

	void set_visible(bool value=true) { is_visible_ = value; }
	bool is_visible() const { return is_visible_; }

    virtual void move_to(float x, float y, float z);
    virtual void move_forward(float amount);
    
    virtual void rotate_x(float amount);        
    virtual void rotate_y(float amount);
    virtual void rotate_z(float amount);

    void set_parent(Object* p) {
        Object* old_parent = nullptr;
        if(has_parent()) {
            old_parent = &parent();
            parent().detach_child(this);
        }

        if(p) {
            p->attach_child(this);
        } else {
			//Clean up?
		}

        on_parent_set(old_parent); //Signal
    }

    bool has_parent() const { return parent_ != nullptr; }
    Object& parent() { assert(parent_); return *parent_; }
    Object& child(uint32_t i);

    kmVec3& position() { return position_; }
    kmQuaternion& rotation() { return rotation_; }

    template<typename T>
    T* root_as() {
        Object* self = this;
        while(self->has_parent()) {
            self = &self->parent();
        }

        return dynamic_cast<T*>(self);
    }

    virtual void accept(ObjectVisitor& visitor) = 0;
    virtual void on_parent_set(Object* old_parent) {}

    uint64_t id() const { return id_; }
    
    Scene& scene();
    
    void set_user_data(void* data) { user_data_ = data; }
    void* user_data() const { return user_data_; }
};

}

#endif // OBJECT_H_INCLUDED
