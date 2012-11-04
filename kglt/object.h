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
    Object(Scene* parent_scene);
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

    uint64_t uuid() const { return uuid_; }
        
    virtual void _initialize(Scene& scene) {}
    virtual void do_update(double dt) {}

    Scene& scene() { return *scene_; }
    const Scene& scene() const { return *scene_; }

    virtual void destroy() {}
    void destroy_children() {
        //If this looks weird, it's because when you destroy
        //children the index changes so you need to gather them
        //up first and then destroy them
        std::vector<Object*> to_destroy;
        for(uint32_t i = 0; i < child_count(); ++i) {
            to_destroy.push_back(&child(i));
        }
        for(Object* o: to_destroy) {
            o->destroy();
        }
    }

protected:
    void update_from_parent() {
        if(!has_parent()) {
            kmVec3Assign(&absolute_position_, &position_);
            kmQuaternionAssign(&absolute_orientation_, &rotation_);
        } else {
            kmVec3Add(&absolute_position_, &parent().absolute_position_, &position_);
            kmQuaternionAdd(&absolute_orientation_, &parent().absolute_orientation_, &rotation_);
        }

        std::for_each(children().begin(), children().end(), [](Object* x) { x->update_from_parent(); });
    }

private:
    static uint64_t object_counter;
    uint64_t uuid_;

    Scene* scene_; //Each object is owned by a scene

    kmVec3 position_;
    kmQuaternion rotation_;

    kmVec3 absolute_position_;
    kmQuaternion absolute_orientation_;

    void parent_changed_callback(Object* old_parent, Object* new_parent) {
        update_from_parent();
    }

    bool is_visible_;
};

}

#endif // OBJECT_H_INCLUDED
