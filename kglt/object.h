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

#include "kazmath/mat4.h"
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
    Object(SubScene* parent_scene);
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

    kmMat4 absolute_transformation() {
        kmMat4 transform;
        kmMat4RotationQuaternion(&transform, &absolute_orientation_);
        kmMat4Translation(&transform, absolute_position().x, absolute_position().y, absolute_position().z);
        return transform;
    }

    void set_position(const kmVec3& pos) {
        kmVec3Assign(&position_, &pos);
        update_from_parent();
    }

    const kmVec3& position() const { return position_; }
    const kmVec3& absolute_position() const { return absolute_position_; }

    kmQuaternion& rotation() { return rotation_; }

    uint64_t uuid() const { return uuid_; }
        
    virtual void _initialize() {}
    virtual void do_update(double dt) {
    }

    SubScene& subscene() { return *subscene_; }
    const SubScene& subscene() const { return *subscene_; }

    virtual void destroy() = 0;

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

    SubScene* subscene_; //Each object is owned by a scene

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
