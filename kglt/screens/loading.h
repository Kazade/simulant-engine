#ifndef LOADING_H
#define LOADING_H

#include "../generic/managed.h"

namespace kglt {

class Scene;

namespace screens {

class Loading:
    public Managed<Loading> {

public:
    Loading(Scene& scene);

    void activate();
    void deactivate();

    void update(float dt);

    bool is_active() const;

private:
    Scene& scene_;
    bool is_active_;
};

}
}

#endif // LOADING_H
