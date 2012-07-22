#ifndef KGLT_OBJECT_H_INCLUDED
#define KGLT_OBJECT_H_INCLUDED

#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <tr1/memory>

#include <boost/any.hpp>

#include "object_visitor.h"
#include "kazmath/vec3.h"
#include "kazmath/quaternion.h"
#include "types.h"

namespace kglt {


class Scene;

class Object {
private:
    static uint64_t object_counter;
    uint64_t id_;

    kmVec3 position_;
    kmQuaternion rotation_;

    boost::any user_data_;
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


    bool is_visible_;
public:
    typedef std::tr1::shared_ptr<Object> ptr;

    Object():
        id_(++object_counter),
        parent_(nullptr),
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
    const Object& parent() const { assert(parent_); return *parent_; }

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

    template<typename T>
    T* root_as() const {
        const Object* self = this;
        while(self->has_parent()) {
            self = &self->parent();
        }

        return dynamic_cast<T*>(self);
    }

    virtual void pre_visit(ObjectVisitor& visitor) {}
    virtual void post_visit(ObjectVisitor& visitor) {}

    virtual void accept(ObjectVisitor& visitor) = 0;
    virtual void on_parent_set(Object* old_parent) {}

    uint64_t id() const { return id_; }
    
    Scene& scene();
    const Scene& scene() const;
    
    void set_user_data(boost::any data) { 
        user_data_ = data; 
    }

    template<typename T>
    T user_data() const { 
        assert(!user_data_.empty());
        return boost::any_cast<T>(user_data_);
    }
    
    bool has_user_data() const { return !user_data_.empty(); }

    virtual void _initialize(Scene& scene) {}
    
protected:    
    template<typename T>
    void do_accept(T* _this, ObjectVisitor& visitor) {
		if(!visitor.pre_visit(_this)) {
		    return;
		}

        if(_this->is_visible()) {
            visitor.visit(_this);
        }

        for(Object* child: _this->children_) {
            child->accept(visitor);
        }

        visitor.post_visit(_this);    
    }
};

}

#endif // OBJECT_H_INCLUDED
