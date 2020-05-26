#pragma once

#include "../signals/signal.h"

namespace smlt {

typedef sig::signal<void ()> DestroyedSignal;

class DestroyableObject {
    // Fired when destroy() is called
    DEFINE_SIGNAL(DestroyedSignal, signal_destroyed);

    template<typename PolyType, typename IDType, typename T, typename ...SubTypes>
    friend class StageNodeManager;

    bool is_marked_for_destruction_ = false;

public:
    virtual ~DestroyableObject() {}

    virtual void destroy() = 0;
    virtual void destroy_immediately() = 0;

    bool is_marked_for_destruction() const {
        return is_marked_for_destruction_;
    }
};

class Stage;

template<typename T, typename Owner>
class TypedDestroyableObject : public virtual DestroyableObject {
public:
    TypedDestroyableObject(Owner* owner):
        owner_(owner) {}

    void destroy() override {
        signal_destroyed()();
        owner_->destroy_object((T*) this);
    }

    void destroy_immediately() override {
        signal_destroyed()();
        owner_->destroy_object_immediately((T*) this);
    }

private:
    Owner* owner_ = nullptr;
};

}
