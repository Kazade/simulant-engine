#ifndef KGLT_OBJECT_H_INCLUDED
#define KGLT_OBJECT_H_INCLUDED

#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <tr1/memory>
#include <stdexcept>
#include <boost/any.hpp>

#include "generic/tree.h"
#include "generic/user_data_carrier.h"

#include "object_visitor.h"
#include "kazmath/vec3.h"
#include "kazmath/quaternion.h"
#include "types.h"

namespace kglt {

class Scene;

class Object :
    public generic::TreeNode<Object>, //Objects form a tree
    public generic::UserDataCarrier { //And they allow additional data to be attached

public:
    typedef std::tr1::shared_ptr<Object> ptr;

    Object();
    virtual ~Object();

	virtual void update(double dt) {
		do_update(dt);
		
        for(uint32_t i = 0; i < child_count(); ++i) {
            Object& c = child(i);
            c.update(dt);
		}
	}

	void set_visible(bool value=true) { is_visible_ = value; }
	bool is_visible() const { return is_visible_; }

    virtual void move_to(float x, float y, float z);
    virtual void move_forward(float amount);
    
    virtual void rotate_x(float amount);        
    virtual void rotate_y(float amount);
    virtual void rotate_z(float amount);

    kmVec3& position() { return position_; }
    kmQuaternion& rotation() { return rotation_; }

    virtual void pre_visit(ObjectVisitor& visitor) {}
    virtual void post_visit(ObjectVisitor& visitor) {}

    virtual void accept(ObjectVisitor& visitor) = 0;
    virtual void on_parent_set(Object* old_parent) {}

    uint64_t id() const { return id_; }
    
    Scene& scene();
    const Scene& scene() const;
    
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

        for(uint32_t i = 0; i < child_count(); ++i) {
            Object& c = child(i);
            c.accept(visitor);
        }

        visitor.post_visit(_this);    
    }

    virtual void do_update(double dt) {}

private:
    static uint64_t object_counter;
    uint64_t id_;

    kmVec3 position_;
    kmQuaternion rotation_;

    kmVec3 absolute_position_;
    kmQuaternion absolute_orientation_;

    void parent_changed_callback(Object* old_parent, Object* new_parent) {
        update_from_parent();
    }

    void update_from_parent() {
        if(!has_parent()) {
            kmVec3Assign(&absolute_position_, &position_);
            kmQuaternionAssign(&absolute_orientation_, &rotation_);
            return;
        }

        kmVec3Add(&absolute_position_, &parent().absolute_position_, &position_);
        kmQuaternionAdd(&absolute_orientation_, &parent().absolute_orientation_, &rotation_);
    }

    bool is_visible_;

};

}

#endif // OBJECT_H_INCLUDED
