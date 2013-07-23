#ifndef PATH_FOLLOWER_H
#define PATH_FOLLOWER_H

#include "../../kazbase/exceptions.h"
#include "../../generic/managed.h"
#include "../../base.h"
#include "path.h"

#include "OpenSteer/AbstractVehicle.h"
#include "OpenSteer/SteerLibrary.h"

namespace kglt {
namespace extra {

class Path;

template<typename Derived>
class PathFollowerBase:
    public OpenSteer::AbstractVehicle {

public:
    //OpenSteer overrides

    //Pass-throughs to MoveableActorHolder
    kglt::Vec3 position() const { return ((Derived*)this)->actor_->position(); }
    kglt::Vec3 setPosition(kglt::Vec3 p) { ((Derived*)this)->actor_->set_position(p); return position();  }
    kglt::Vec3 velocity() const { return ((Derived*)this)->actor_->velocity(); }
    float maxForce() const { return ((Derived*)this)->actor_->max_force(); }
    float speed() const { return velocity().length(); }
    float maxSpeed() const { return ((Derived*)this)->actor_->max_speed(); }
    float radius() const { return ((Derived*)this)->actor_->radius(); }
    float mass() const { return ((Derived*)this)->actor_->mass(); }

    Vec3 side() const {
        Vec3 result;
        kmQuaternion rot = ((Derived*)this)->actor_->rotation();
        kmQuaternionGetRightVec3(&result, &rot);
        return result;
    }

    Vec3 up() const {
        Vec3 result;
        kmQuaternion rot = ((Derived*)this)->actor_->rotation();
        kmQuaternionGetUpVec3(&result, &rot);
        return result;
    }

    Vec3 forward() const {
        Vec3 result;
        kmQuaternion rot = ((Derived*)this)->actor_->rotation();
        kmQuaternionGetForwardVec3RH(&result, &rot);
        return result;
    }

    Vec3 predictFuturePosition(const float predictionTime) const {
        return position() + (velocity() * predictionTime);
    }

    //Always true in KGLT
    bool rightHanded() const { return true; }

    //The following OpenSteer overrides are only used in the OpenSteer
    //demo app which isn't included.
    float setMaxForce(float) { throw NotImplementedError(__FILE__, __LINE__); }
    float setSpeed(float) { throw NotImplementedError(__FILE__, __LINE__); }
    float setRadius(float)  { throw NotImplementedError(__FILE__, __LINE__); }
    float setMass(float) { throw NotImplementedError(__FILE__, __LINE__); }
    float setMaxSpeed(float) { throw NotImplementedError(__FILE__, __LINE__); }
    Vec3 setSide(Vec3 s) { throw NotImplementedError(__FILE__, __LINE__); }
    Vec3 setUp(Vec3 u) { throw NotImplementedError(__FILE__, __LINE__); }
    Vec3 setForward(Vec3 f) { throw NotImplementedError(__FILE__, __LINE__); }
    void setUnitSideFromForwardAndUp() { throw NotImplementedError(__FILE__, __LINE__); }
    void resetLocalSpace() { throw NotImplementedError(__FILE__, __LINE__); }
    Vec3 localizeDirection(const Vec3 &globalDirection) const {
        throw NotImplementedError(__FILE__, __LINE__);
    }
    Vec3 globalizeDirection(const Vec3 &localDirection) const {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    Vec3 localizePosition(const Vec3 &globalPosition) const {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    Vec3 globalizePosition(const Vec3 &localPosition) const {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    void regenerateOrthonormalBasis(const kglt::Vec3& newForward, const kglt::Vec3& newUp) {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    void regenerateOrthonormalBasis(const Vec3 &newForward) {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    void regenerateOrthonormalBasisUF(const Vec3 &newUnitForward) {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    Vec3 localRotateForwardToSide(const Vec3 &v) const {
        throw NotImplementedError(__FILE__, __LINE__);
    }

    Vec3 globalRotateForwardToSide(const Vec3 &globalForward) const {
        throw NotImplementedError(__FILE__, __LINE__);
    }

};

class PathFollower:
    public Managed<PathFollower>,
    public OpenSteer::SteerLibraryMixin<PathFollowerBase<PathFollower> > {

public:
    PathFollower(MoveableActorHolder* parent, float max_speed, float max_force);
    void follow(Path path);
    void enable_debug(bool value=true);

    kglt::Vec3 force_to_apply(const kglt::Vec3& velocity);

private:
    kglt::Vec3 seek(const kglt::Vec3& target, const Vec3 &velocity) const;

    MoveableActorHolder* actor_;
    float max_speed_;
    float max_force_;

    Path path_;
    bool loop_ = false;

    kglt::MeshID debug_mesh_;
    kglt::ActorID debug_actor_;
    kglt::SubMeshIndex normal_points_mesh_;

    mutable std::vector<kglt::Vec3> normal_points_;
    void update_debug_mesh() const;

    friend class PathFollowerBase<PathFollower>;
};

}
}

#endif // PATH_FOLLOWER_H
