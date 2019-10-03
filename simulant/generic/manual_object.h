#pragma once

namespace smlt {

class DestroyableObject {
public:
    virtual ~DestroyableObject() {}

    virtual void destroy() = 0;
    virtual void destroy_immediately() = 0;
};

class Stage;

template<typename T, typename Owner>
class TypedDestroyableObject : public virtual DestroyableObject {
public:
    TypedDestroyableObject(Owner* owner):
        owner_(owner) {}

    void destroy() override {
        owner_->destroy_object((T*) this);
    }

    void destroy_immediately() override {
        owner_->destroy_object_immediately((T*) this);
    }

private:
    Owner* owner_ = nullptr;
};

}
