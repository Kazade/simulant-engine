#ifndef BASE_H
#define BASE_H

#include "stage.h"
#include "window_base.h"
#include "types.h"

namespace kglt {

class ActorHolder {
public:
    ActorHolder(WindowBase& window):
        window_(window) {}

    virtual StageID stage_id() const = 0;
    virtual ActorID actor_id() const = 0;

    StagePtr stage() { return window_.stage(stage_id()); }
    ActorPtr actor() { return stage()->actor(actor_id()); }

    const StagePtr stage() const { return window_.stage(stage_id()); }
    const ActorPtr actor() const { return stage()->actor(actor_id()); }

private:
    WindowBase& window_;
};

class MoveableActorHolder:
    public ActorHolder {

public:
    MoveableActorHolder(WindowBase &window);

    void set_position(const kglt::Vec3& position);
    kglt::Vec3 position() const;

    void set_rotation(const Quaternion &quaternion);
    Quaternion rotation() const;

    virtual void set_velocity(const kglt::Vec3& vel);
    virtual kglt::Vec3 velocity() const;

    virtual void set_mass(float mass);
    virtual float mass() const;

    virtual void set_min_speed(float speed);
    virtual float min_speed() const;

    virtual void set_max_speed(float speed);
    virtual float max_speed() const;

    virtual void set_max_force(float force);
    virtual float max_force() const;

    virtual void set_radius(float radius);
    virtual float radius() const;

private:
    kglt::Vec3 velocity_;
    float mass_ = 1.0;
    float max_speed_ = 1.0;
    float min_speed_ = 0.0;
    float max_force_ = 1.0;
    float radius_ = 1.0;
};

}

#endif // BASE_H
