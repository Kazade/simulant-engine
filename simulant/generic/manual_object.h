#pragma once

#include "../signals/signal.h"

namespace smlt {

typedef sig::signal<void ()> DestroyedSignal;

class DestroyableObject {
public:
    // Fired when destroy() is called
    DEFINE_SIGNAL(DestroyedSignal, signal_destroyed);

    friend class StageNodeManager;

private:
    bool destroyed_ = false;

    /*
     * Called when destroy() is called, if the object hasn't already
     * been destroyed. Return false to cancel destruction */
    virtual bool on_destroy() = 0;

    /* Private functions, do not override */
    virtual void finalize_destroy() {}
    virtual void finalize_destroy_immediately() {}
public:
    virtual ~DestroyableObject() {}

    bool destroy() {
        if(destroyed_) {
            return false;
        }

        destroyed_ = on_destroy();
        if(destroyed_) {
            signal_destroyed()();
            finalize_destroy();
        }

        return destroyed_;
    }

    bool destroy_immediately() {
        if(destroyed_) {
            return false;
        }

        destroyed_ = on_destroy();
        if(destroyed_) {
            signal_destroyed()();
            finalize_destroy_immediately();
        }

        return destroyed_;
    }

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

private:
    bool on_destroy() override {
        if(owner_) {
            signal_destroyed()();
            owner_->destroy_object((T*) this);
            return true;
        }
        return false;
    }

    bool on_destroy_immediately() override {
        if(owner_) {
            signal_destroyed()();
            owner_->destroy_object_immediately((T*) this);
            return true;
        }

        return false;
    }

    Owner* owner_ = nullptr;
};

}
