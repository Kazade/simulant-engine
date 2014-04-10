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
    Loading(WindowBase& window);

    void activate();
    void deactivate();

    void update(float dt);

    bool is_active() const;

    bool init() override;
    void cleanup() override;

protected:
    WindowBase& window() { return window_; }

private:
    WindowBase& window_;
    bool is_active_;

    UIStageID stage_;
    CameraID camera_;

    PipelineID pipeline_;

    std::vector<PipelineID> stashed_pipelines_;
};

}
}

#endif // LOADING_H
