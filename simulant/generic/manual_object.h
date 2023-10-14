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
    virtual bool on_destroy() { return true; }

    /* Private functions, do not override */
    virtual void finalize_destroy() {}
    virtual void finalize_destroy_immediately() {}
public:
    virtual ~DestroyableObject() {
        if(!destroyed_) {
            signal_destroyed()();
        }
    }

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

private:
    void finalize_destroy() override {
        if(owner_) {
            owner_->destroy_object((T*) this);
        }
    }

    void finalize_destroy_immediately() override {
        if(owner_) {
            owner_->destroy_object_immediately((T*) this);
        }
    }

    Owner* owner_ = nullptr;
};

}
