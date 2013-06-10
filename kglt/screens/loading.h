#ifndef LOADING_H
#define LOADING_H

#include <vector>

#include "../generic/managed.h"
#include "../types.h"

namespace kglt {

class Scene;

namespace screens {

class Loading:
    public Managed<Loading> {

public:
    Loading(Scene& scene);
    ~Loading();

    void activate();
    void deactivate();

    void update(float dt);

    bool is_active() const;

private:
    Scene& scene_;
    bool is_active_;

    StageID stage_;
    CameraID camera_;
    ActorID cube_;

    PipelineID pipeline_;

    std::vector<PipelineID> stashed_pipelines_;
};

}
}

#endif // LOADING_H
