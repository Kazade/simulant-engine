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
#include "generic/visitor.h"

#include "kazmath/vec3.h"
#include "kazmath/quaternion.h"
#include "types.h"

namespace kglt {

class Scene;

class Object :
    public generic::TreeNode<Object>, //Objects form a tree
    public generic::UserDataCarrier,
    public generic::VisitableBase<Object> { //And they allow additional data to be attached

public:
    VIS_DEFINE_VISITABLE();

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
    kmVec3& absolute_position() { return absolute_position_; }

    kmQuaternion& rotation() { return rotation_; }

    virtual void on_parent_set(Object* old_parent) {}

    uint64_t id() const { return id_; }
    
    Scene& scene();
    const Scene& scene() const;
    
    virtual void _initialize(Scene& scene) {}
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

        std::for_each(children().begin(), children().end(), [](Object* x) { x->update_from_parent(); });
    }

    bool is_visible_;

};

}

#endif // OBJECT_H_INCLUDED
