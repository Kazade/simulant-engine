#include "../../generic/managed.h"

#include "ode_engine.h"
#include "ode_body.h"
#include "ode_collidable.h"

#include "../types.h"

namespace kglt {
namespace physics {

static const int MAX_CONTACTS = 3;

static void global_near_callback(void *data, dGeomID o1, dGeomID o2) {
    ODEEngine* _this = (ODEEngine*) data;

    _this->near_callback(o1, o2);
}

void ODEEngine::near_callback(dGeomID o1, dGeomID o2) {
    ODECollidable* c1 = (ODECollidable*) dGeomGetData(o1);
    ODECollidable* c2 = (ODECollidable*) dGeomGetData(o2);

    dContact contact[MAX_CONTACTS];

    int collision_count = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact));

    if(c1->is_ghost() || c2->is_ghost()) {
        //Don't respond if we're not supposed to (e.g. we're a hit zone or something)
        return;
    }

    for(int32_t i = 0; i < collision_count; ++i) {
        //Calculate combined surface values to build contact joints

        contact[i].surface.mode = dContactBounce;
        contact[i].surface.bounce = c1->bounciness() * c2->bounciness();

        if(c1->has_infinite_friction() || c2->has_infinite_friction()) {
            contact[i].surface.mu = dInfinity;
        } else {
            contact[i].surface.mu = c1->friction() * c2->friction();
        }

        dBodyID body1 = dGeomGetBody(contact[i].geom.g1);
        dBodyID body2 = dGeomGetBody(contact[i].geom.g2);

        if(body1 == body2) {
            //Don't collide the same body against itself
            continue;
        }

        if(i == 0) {
            //Fire the collision signal on the first loop only
            c1->signal_collided_(*c2);
            c2->signal_collided_(*c1);
        }

        dJointID c = dJointCreateContact(world(), contact_group_, &contact[i]);
        dJointAttach(c, body1, body2);
    }
}

std::shared_ptr<ResponsiveBody> ODEEngine::new_responsive_body(Object* owner) {
    auto result = std::make_shared<ODEBody>(owner);
    if(!result->init()) {
        throw InstanceInitializationError();
    }
    return result;
}

std::shared_ptr<Collidable> ODEEngine::new_collidable(Object* owner) {
    auto result = std::make_shared<ODECollidable>(owner, this);
    if(!result->init()) {
        throw InstanceInitializationError();
    }
    return result;
}

ShapeID ODEEngine::create_plane(float a, float b, float c, float d) {
    auto new_c = new_collidable(nullptr);

    ShapeID new_id = new_c->add_plane(a, b, c, d);

    shapes_[new_id] = new_c;
    return new_id;
}

void ODEEngine::set_gravity(const Vec3& gravity) {
    dWorldSetGravity(world_, gravity.x, gravity.y, gravity.z);
}

void ODEEngine::step(double dt) {
    dJointGroupEmpty(contact_group_);
    dSpaceCollide(space_, this, &global_near_callback);
    dWorldStep(world_, dt);    
}

}
}
