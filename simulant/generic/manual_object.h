#pragma once

#include "../signals/signal.h"

namespace smlt {

typedef sig::signal<void ()> DestroyedSignal;

class DestroyableObject {
public:
    // Fired when destroy() is called
    DEFINE_SIGNAL(DestroyedSignal, signal_destroyed);

    template<typename PolyType, typename IDType, typename T, typename ...SubTypes>
    friend class StageNodeManager;

protected:
    bool destroyed_ = false;

public:
    virtual ~DestroyableObject() {}

    virtual bool destroy() = 0;
    virtual bool destroy_immediately() = 0;

    bool is_destroyed() const {
        return destroyed_;
    }
};

class Stage;

template<typename T, typename Owner>
class TypedDestroyableObject : public virtual DestroyableObject {
public:
    friend Owner;

    TypedDestroyableObject(Owner* owner):
        owner_(owner) {}

    virtual ~TypedDestroyableObject() {
        if(!destroyed_) {
            signal_destroyed()();
        }
    }

    bool destroy() override {
        if(!destroyed_ && owner_) {
            signal_destroyed()();
            destroyed_ = true;
            owner_->destroy_object((T*) this);
            return true;
        }
        return false;
    }

    bool destroy_immediately() override {
        if(!destroyed_ && owner_) {
            signal_destroyed()();
            destroyed_ = true;
            owner_->destroy_object_immediately((T*) this);
            return true;
        }

        return false;
    }

private:
    Owner* owner_ = nullptr;
};

}
