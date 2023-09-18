#pragma once

#include "../interfaces/updateable.h"
#include "../interfaces/nameable.h"
#include "../generic/manual_object.h"

namespace smlt {

class Service:
    public virtual Nameable,
    public DestroyableObject,
    public Updateable {

public:
    void update(float dt) override final;
    void late_update(float dt) override final;
    void fixed_update(float step) override final;

private:
    virtual void on_update(float dt) { _S_UNUSED(dt); }
    virtual void on_fixed_update(float step) { _S_UNUSED(step); }
    virtual void on_late_update(float dt) { _S_UNUSED(dt); }
};

}
