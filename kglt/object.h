#ifndef KGLT_OBJECT_H_INCLUDED
#define KGLT_OBJECT_H_INCLUDED

#include <iosfwd>
#include <cassert>
#include <vector>
#include <tr1/memory>
#include <stdexcept>
#include <boost/any.hpp>
#include <sigc++/sigc++.h>

#include "generic/tree.h"
#include "generic/data_carrier.h"
#include "generic/visitor.h"

#include "kazmath/mat4.h"
#include "kazmath/vec3.h"
#include "kazmath/quaternion.h"
#include "types.h"

namespace kglt {

class Scene;

class Object :
    public generic::TreeNode<Object>, //Objects form a tree
    public generic::DataCarrier,
    public generic::VisitableBase<Object> { //And they allow additional data to be attached

public:
    Object(Stage* parent_scene);
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
    virtual void move_to(const kmVec3& pos) { move_to(pos.x, pos.y, pos.z); }
    virtual void move_forward(float amount);
    
    virtual void rotate_x(float amount);        
    virtual void rotate_y(float amount);
    virtual void rotate_z(float amount);
    virtual void rotate_to(float angle, float x, float y, float z);
    virtual void rotate_to(const kmQuaternion& quat);

    //Make this object ignore parent rotations or rotate commands until unlocked
    void lock_rotation(float angle, float x, float y, float z);
    void unlock_rotation();

    //Make this object ignore parent translations or move commands until unlocked
    void lock_position(float x, float y, float z);
    void unlock_position();

    kmMat4 absolute_transformation();

    const kmVec3& position() const { return position_; }
    const kmVec3& absolute_position() const { return absolute_position_; }

    const kmQuaternion& rotation() { return rotation_; }
    const kmQuaternion& absolute_rotation() { return absolute_orientation_; }

    uint64_t uuid() const { return uuid_; }
        
    virtual void _initialize() {}
    virtual void do_update(double dt) {}

    Stage& stage() { return *stage_; }
    const Stage& stage() const { return *stage_; }

    virtual void destroy() = 0;

    void destroy_children();

protected:
    void update_from_parent();
    void set_position(const kmVec3& pos);

    kmVec3 position_;
    kmQuaternion rotation_;
private:
    static uint64_t object_counter;
    uint64_t uuid_;

    Stage* stage_; //Each object is owned by a scene


    kmVec3 absolute_position_;
    kmQuaternion absolute_orientation_;

    sigc::connection parent_changed_connection_;

    void parent_changed_callback(Object* old_parent, Object* new_parent) {
        update_from_parent();
    }

    bool is_visible_;
    bool rotation_locked_;
    bool position_locked_;

    virtual void transformation_changed() {}
};

}

#endif // OBJECT_H_INCLUDED
