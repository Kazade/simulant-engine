#ifndef BASE_H
#define BASE_H

#include "scene.h"
#include "stage.h"

#include "types.h"

namespace kglt {

class ActorHolder {
public:
    ActorHolder(Scene& scene):
        scene_(scene) {}

    Scene& scene() { return scene_; }

    virtual StageID stage_id() const = 0;
    virtual ActorID actor_id() const = 0;

    Stage* stage() { return &scene_.stage(stage_id()); }
    Actor* actor() { return &stage()->actor(actor_id()); }

    const Stage* stage() const { return &scene_.stage(stage_id()); }
    const Actor* actor() const { return &stage()->actor(actor_id()); }

private:
    Scene& scene_;
};

class MoveableActorHolder:
    public ActorHolder {

public:
    MoveableActorHolder(Scene& scene);

    void set_position(const kglt::Vec3& position);
    kglt::Vec3 position() const;

    void set_rotation(const kmQuaternion& quaternion);
    kmQuaternion rotation() const;

    virtual void set_velocity(const kglt::Vec3& vel);
    virtual kglt::Vec3 velocity() const;

    virtual void set_mass(float mass);
    virtual float mass() const;

    virtual void set_max_speed(float speed);
    virtual float max_speed() const;

    virtual void set_max_force(float force);
    virtual float max_force() const;

    virtual void set_radius(float radius);
    virtual float radius() const;

private:
    kglt::Vec3 velocity_;
    float mass_;
    float max_speed_;
    float max_force_;
    float radius_;
};

}

#endif // BASE_H
